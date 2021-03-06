/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * The Original Code is Copyright (C) 2019 Blender Foundation.
 * All rights reserved.
 */

/** \file
 * \ingroup depsgraph
 */

#pragma once

#include "DNA_ID.h"

#include "intern/eval/deg_eval_runtime_backup_animation.h"
#include "intern/eval/deg_eval_runtime_backup_movieclip.h"
#include "intern/eval/deg_eval_runtime_backup_object.h"
#include "intern/eval/deg_eval_runtime_backup_scene.h"
#include "intern/eval/deg_eval_runtime_backup_sound.h"

namespace DEG {

struct Depsgraph;

class RuntimeBackup {
 public:
  explicit RuntimeBackup(const Depsgraph *depsgraph);

  /* NOTE: Will reset all runtime fields which has been backed up to nullptr. */
  void init_from_id(ID *id);

  /* Restore fields to the given ID. */
  void restore_to_id(ID *id);

  AnimationBackup animation_backup;
  SceneBackup scene_backup;
  SoundBackup sound_backup;
  ObjectRuntimeBackup object_backup;
  DrawDataList drawdata_backup;
  DrawDataList *drawdata_ptr;
  MovieClipBackup movieclip_backup;
};

}  // namespace DEG
