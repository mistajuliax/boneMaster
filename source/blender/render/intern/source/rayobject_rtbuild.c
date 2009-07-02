#include <assert.h>
#include <math.h>

#include "rayobject_rtbuild.h"
#include "MEM_guardedalloc.h"
#include "BLI_arithb.h"
#include "BKE_utildefines.h"

static int partition_nth_element(RTBuilder *b, int _begin, int _end, int n);
static void split_leafs(RTBuilder *b, int *nth, int partitions, int split_axis);


static void rtbuild_init(RTBuilder *b, RayObject **begin, RayObject **end)
{
	int i;

	b->begin = begin;
	b->end   = end;
	b->split_axis = 0;
	
	for(i=0; i<RTBUILD_MAX_CHILDS; i++)
		b->child_offset[i] = 0;
}

RTBuilder* rtbuild_create(int size)
{
	RTBuilder *builder  = (RTBuilder*) MEM_mallocN( sizeof(RTBuilder), "RTBuilder" );
	RayObject **memblock= (RayObject**)MEM_mallocN( sizeof(RayObject*)*size,"RTBuilder.objects");
	rtbuild_init(builder, memblock, memblock);
	return builder;
}

void rtbuild_free(RTBuilder *b)
{
	MEM_freeN(b->begin);
	MEM_freeN(b);
}

void rtbuild_add(RTBuilder *b, RayObject *o)
{
	*(b->end++) = o;
}

RTBuilder* rtbuild_get_child(RTBuilder *b, int child, RTBuilder *tmp)
{
	rtbuild_init( tmp, b->begin + b->child_offset[child], b->begin + b->child_offset[child+1] );
	return tmp;
}

int rtbuild_size(RTBuilder *b)
{
	return b->end - b->begin;
}

/* Split methods */
static void merge_bb(RTBuilder *b, float *min, float *max)
{
	RayObject **index = b->begin;

	for(; index != b->end; index++)
		RE_rayobject_merge_bb(*index, min, max);
}

int rtbuild_get_largest_axis(RTBuilder *b)
{
	float min[3], max[3], sub[3];

	INIT_MINMAX(min, max);
	merge_bb( b, min, max);
	
	sub[0] = max[0]-min[0];
	sub[1] = max[1]-min[1];
	sub[2] = max[2]-min[2];
	if(sub[0] > sub[1])
	{
		if(sub[0] > sub[2])
			return 0;
		else
			return 2;
	}
	else
	{
		if(sub[1] > sub[2])
			return 1;
		else
			return 2;
	}	
}


//Left balanced tree
int rtbuild_mean_split(RTBuilder *b, int nchilds, int axis)
{
	int i;
	int mleafs_per_child, Mleafs_per_child;
	int tot_leafs  = rtbuild_size(b);
	int missing_leafs;

	long long s;

	assert(nchilds <= RTBUILD_MAX_CHILDS);
	
	//TODO optimize calc of leafs_per_child
	for(s=nchilds; s<tot_leafs; s*=nchilds);
	Mleafs_per_child = s/nchilds;
	mleafs_per_child = Mleafs_per_child/nchilds;
	
	//split min leafs per child	
	b->child_offset[0] = 0;
	for(i=1; i<=nchilds; i++)
		b->child_offset[i] = mleafs_per_child;
	
	//split remaining leafs
	missing_leafs = tot_leafs - mleafs_per_child*nchilds;
	for(i=1; i<=nchilds; i++)
	{
		if(missing_leafs > Mleafs_per_child - mleafs_per_child)
		{
			b->child_offset[i] += Mleafs_per_child - mleafs_per_child;
			missing_leafs -= Mleafs_per_child - mleafs_per_child;
		}
		else
		{
			b->child_offset[i] += missing_leafs;
			missing_leafs = 0;
			break;
		}
	}
	
	//adjust for accumulative offsets
	for(i=1; i<=nchilds; i++)
		b->child_offset[i] += b->child_offset[i-1];

	//Count created childs
	for(i=nchilds; b->child_offset[i] == b->child_offset[i-1]; i--);
	split_leafs(b, b->child_offset, i, axis);
	
	assert( b->child_offset[0] == 0 && b->child_offset[i] == tot_leafs );
	return i;
}
	
	
int rtbuild_mean_split_largest_axis(RTBuilder *b, int nchilds)
{
	int axis = rtbuild_get_largest_axis(b);
	return rtbuild_mean_split(b, nchilds, axis);
}


/*
 * Helper code
 * PARTITION code / used on mean-split
 * basicly this a std::nth_element (like on C++ STL algorithm)
 */
static void sort_swap(RTBuilder *b, int i, int j)
{
	SWAP(RayObject*, b->begin[i], b->begin[j]);
}
 
static float sort_get_value(RTBuilder *b, int i)
{
	float min[3], max[3];
	INIT_MINMAX(min, max);
	RE_rayobject_merge_bb(b->begin[i], min, max);
	return max[b->split_axis];
}
 
static int medianof3(RTBuilder *d, int a, int b, int c)
{
	float fa = sort_get_value( d, a );
	float fb = sort_get_value( d, b );
	float fc = sort_get_value( d, c );

	if(fb < fa)
	{
		if(fc < fb)
			return b;
		else
		{
			if(fc < fa)
				return c;
			else
				return a;
		}
	}
	else
	{
		if(fc < fb)
		{
			if(fc < fa)
				return a;
			else
				return c;
		}
		else
			return b;
	}
}

static void insertionsort(RTBuilder *b, int lo, int hi)
{
	int i;
	for(i=lo; i<hi; i++)
	{
		RayObject *t = b->begin[i];
		float tv= sort_get_value(b, i);
		int j=i;
		
		while( j != lo && tv < sort_get_value(b, j-1))
		{
			b->begin[j] = b->begin[j-1];
			j--;
		}
		b->begin[j] = t;
	}
}

static int partition(RTBuilder *b, int lo, int mid, int hi)
{
	float x = sort_get_value( b, mid );
	
	int i=lo, j=hi;
	while (1)
	{
		while(sort_get_value(b,i) < x) i++;
		j--;
		while(x < sort_get_value(b,j)) j--;
		if(!(i < j))
			return i;

		sort_swap(b, i, j);
		i++;
	}
}

//
// PARTITION code / used on mean-split
// basicly this is an adapted std::nth_element (C++ STL <algorithm>)
//
// after a call to this function you can expect one of:
//      every node to left of a[n] are smaller or equal to it
//      every node to the right of a[n] are greater or equal to it
static int partition_nth_element(RTBuilder *b, int _begin, int n, int _end)
{
	int begin = _begin, end = _end, cut;
	while(end-begin > 3)
	{
		cut = partition(b, begin, medianof3(b, begin, begin+(end-begin)/2, end-1), end);
		if(cut <= n)
			begin = cut;
		else
			end = cut;
	}
	insertionsort(b, begin, end);

	return n;
}

static void split_leafs(RTBuilder *b, int *nth, int partitions, int split_axis)
{
	int i;
	b->split_axis = split_axis;

	for(i=0; i < partitions-1; i++)
	{
		assert(nth[i] < nth[i+1] && nth[i+1] < nth[partitions]);

		partition_nth_element(b, nth[i],  nth[i+1], nth[partitions] );
	}
}
