/*
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef AUDIO_ROUTE_H
#define AUDIO_ROUTE_H

/* Initialises and frees the audio routes */
struct audio_route *audio_route_init(void);
void audio_route_free(struct audio_route *ar);

/* Applies an audio route path by name */
void audio_route_apply_path(struct audio_route *ar, const char *name);

/* Resets the mixer back to its initial state */
void reset_mixer_state(struct audio_route *ar);

/* Updates the mixer with any changed values */
void update_mixer_state(struct audio_route *ar);

#endif
