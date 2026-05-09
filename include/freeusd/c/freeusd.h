/**
 * @file freeusd.h
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * @brief Stable C ABI for FreeUSD (implemented in C++; no OpenUSD/Pixar code).
 *
 * Threading: unless documented otherwise, do not use the same @c FreeusdLayer /
 * @c FreeusdStage from multiple threads concurrently. @c freeusd_last_error_message
 * is thread-local.
 *
 * String ownership: functions returning @c char* allocate with @c malloc; free with
 * @c freeusd_string_free. Functions taking @c const char* do not take ownership.
 */

#ifndef FREEUSD_C_FREEUSD_H
#define FREEUSD_C_FREEUSD_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Symbol visibility for the C API (define @c FREEUSD_C_SHARED + platform exports when building a shared lib). */
#ifndef FREEUSD_C_API
#  define FREEUSD_C_API
#endif

/** Result / error code (0 = success). */
enum FreeusdResult {
  FREEUSD_OK = 0,
  FREEUSD_ERR_INVALID_ARGUMENT = 1,
  FREEUSD_ERR_PARSE = 2,
  FREEUSD_ERR_NOT_FOUND = 3,
  FREEUSD_ERR_INTERNAL = 4,
};

typedef struct FreeusdLayer FreeusdLayer;
typedef struct FreeusdLayerStack FreeusdLayerStack;
typedef struct FreeusdStage FreeusdStage;

/** Project version string (static storage; do not free). */
FREEUSD_C_API const char* freeusd_version_string(void);

/**
 * Last error message for the calling thread, valid until the next FreeUSD C API
 * call on this thread. Never null (empty string if no error was recorded).
 */
FREEUSD_C_API const char* freeusd_last_error_message(void);

/** Free strings returned by FreeUSD C API (e.g. @ref freeusd_layer_save_usda_to_string). */
FREEUSD_C_API void freeusd_string_free(char* s);

/** Create an anonymous in-memory layer. @p identifier may be NULL. */
FREEUSD_C_API FreeusdLayer* freeusd_layer_new_anonymous(const char* identifier);

FREEUSD_C_API void freeusd_layer_free(FreeusdLayer* layer);

/**
 * Replace layer contents by parsing @p usda_text (USDA subset supported by FreeUSD).
 * On failure the layer may be cleared; see @ref freeusd_last_error_message.
 */
FREEUSD_C_API int freeusd_layer_load_usda_from_string(FreeusdLayer* layer, const char* usda_text);

/**
 * Serialize layer to USDA text. Returns malloc'd UTF-8 string or NULL on failure.
 * Free with @ref freeusd_string_free.
 */
FREEUSD_C_API char* freeusd_layer_save_usda_to_string(const FreeusdLayer* layer);

/** Layer identifier (may be empty). Returns malloc'd UTF-8; free with @ref freeusd_string_free. */
FREEUSD_C_API char* freeusd_layer_get_identifier_utf8(const FreeusdLayer* layer);

/**
 * All prim paths with authored data in this layer (stable sort).
 * On @ref FREEUSD_OK, @p *out_paths / @p *out_count use @ref freeusd_path_list_free.
 */
FREEUSD_C_API int freeusd_layer_list_prim_paths(const FreeusdLayer* layer, char*** out_paths, size_t* out_count);

/** Layer `documentation` string (may be empty). Returns malloc'd UTF-8; free with @ref freeusd_string_free. */
FREEUSD_C_API char* freeusd_layer_get_documentation_utf8(const FreeusdLayer* layer);

/** 1 if the layer has @c defaultPrim, 0 if not, negative @ref FreeusdResult on error. */
FREEUSD_C_API int freeusd_layer_has_default_prim(const FreeusdLayer* layer);

/**
 * Root @c defaultPrim name. On @ref FREEUSD_OK, @p *out_prim_name is malloc'd UTF-8; free with @ref freeusd_string_free.
 * @ref FREEUSD_ERR_NOT_FOUND if unset.
 */
FREEUSD_C_API int freeusd_layer_get_default_prim_utf8(const FreeusdLayer* layer, char** out_prim_name);

/**
 * Layer @c subLayers asset paths (order preserved; may be empty).
 * On @ref FREEUSD_OK, @p *out_paths / @p *out_count use @ref freeusd_path_list_free.
 */
FREEUSD_C_API int freeusd_layer_list_sub_layers(const FreeusdLayer* layer, char*** out_paths, size_t* out_count);

/**
 * Build a stage from a root layer (shares ownership of the layer; keep @p layer
 * alive for the stage lifetime, or attach and retain as needed).
 */
FREEUSD_C_API FreeusdStage* freeusd_stage_attach_root_layer(const FreeusdLayer* layer);

/**
 * Ordered layer stack (strongest first). Append layers with @ref freeusd_layer_stack_append
 * then @ref freeusd_stage_attach_layer_stack (layers remain owned by their @c FreeusdLayer handles).
 */
FREEUSD_C_API FreeusdLayerStack* freeusd_layer_stack_new(void);
FREEUSD_C_API void freeusd_layer_stack_free(FreeusdLayerStack* stack);
FREEUSD_C_API void freeusd_layer_stack_clear(FreeusdLayerStack* stack);
FREEUSD_C_API void freeusd_layer_stack_append(FreeusdLayerStack* stack, const FreeusdLayer* layer);

/** Strongest-first layer count for @p stage (same order as composition). */
FREEUSD_C_API int freeusd_stage_compose_layer_count(const FreeusdStage* stage, size_t* out_count);

/**
 * Build a stage from a non-empty stack (strongest layer is first appended). Returns NULL if empty.
 */
FREEUSD_C_API FreeusdStage* freeusd_stage_attach_layer_stack(const FreeusdLayerStack* stack);

/**
 * Open a USDA root file from disk and build a composed stage (optionally stacking authored @c subLayers).
 *
 * @p sublayer_policy: @c 0 = root file only (@c RootLayerSublayersPolicy::None), @c 1 = immediate @c subLayers only
 * (@c Shallow), @c 2 = depth-first nested @c subLayers (@c DepthFirst, default in C++).
 *
 * On success the stage owns all loaded layers internally (not exposed as @c FreeusdLayer). On failure returns NULL;
 * see @ref freeusd_last_error_message.
 */
FREEUSD_C_API FreeusdStage* freeusd_stage_open_from_root_file_utf8(const char* layer_path_utf8, int sublayer_policy);

FREEUSD_C_API void freeusd_stage_free(FreeusdStage* stage);

/**
 * Stage pseudo-root path (typically @c \"/\"). Returns malloc'd UTF-8; free with @ref freeusd_string_free.
 * Returns NULL on error.
 */
FREEUSD_C_API char* freeusd_stage_get_pseudo_root_path_utf8(const FreeusdStage* stage);

/**
 * Composed strongest-first @c startTimeCode (first layer that authors it).
 * On @ref FREEUSD_OK, @p *out_has is 1 and @p *out_value is set, or @p *out_has is 0 if unset.
 */
FREEUSD_C_API int freeusd_stage_get_start_time_code(const FreeusdStage* stage, double* out_value, int* out_has);

/** Composed @c endTimeCode (see @ref freeusd_stage_get_start_time_code). */
FREEUSD_C_API int freeusd_stage_get_end_time_code(const FreeusdStage* stage, double* out_value, int* out_has);

/** Composed @c timeCodesPerSecond (see @ref freeusd_stage_get_start_time_code). */
FREEUSD_C_API int freeusd_stage_get_time_codes_per_second(const FreeusdStage* stage, double* out_value, int* out_has);

/** Composed @c framesPerSecond (see @ref freeusd_stage_get_start_time_code). */
FREEUSD_C_API int freeusd_stage_get_frames_per_second(const FreeusdStage* stage, double* out_value, int* out_has);

/** Composed @c framePrecision (see @ref freeusd_stage_get_start_time_code; @p out_value is int64 for C ABI width). */
FREEUSD_C_API int freeusd_stage_get_frame_precision(const FreeusdStage* stage, int64_t* out_value, int* out_has);

/** Composed @c metersPerUnit (see @ref freeusd_stage_get_start_time_code). */
FREEUSD_C_API int freeusd_stage_get_meters_per_unit(const FreeusdStage* stage, double* out_value, int* out_has);

/**
 * Composed @c upAxis string (strongest layer that authors it).
 * On @ref FREEUSD_OK, @p *out_axis_utf8 is malloc'd UTF-8; free with @ref freeusd_string_free.
 * @ref FREEUSD_ERR_NOT_FOUND if unset.
 */
FREEUSD_C_API int freeusd_stage_get_up_axis_utf8(const FreeusdStage* stage, char** out_axis_utf8);

/**
 * Composed @c primOrder from the strongest layer that authors a non-empty list.
 * On @ref FREEUSD_OK, @p *out_paths / @p *out_count use @ref freeusd_path_list_free (possibly empty).
 */
FREEUSD_C_API int freeusd_stage_list_prim_order_paths_utf8(const FreeusdStage* stage, char*** out_paths,
                                                          size_t* out_count);

/**
 * Sorted union of prim paths across all composed layers.
 * On @ref FREEUSD_OK, @p *out_paths / @p *out_count use @ref freeusd_path_list_free.
 */
FREEUSD_C_API int freeusd_stage_list_composed_prim_paths(const FreeusdStage* stage, char*** out_paths,
                                                         size_t* out_count);

/** 1 if the root (strongest) layer has @c defaultPrim, 0 if not, negative @ref FreeusdResult on error. */
FREEUSD_C_API int freeusd_stage_has_default_prim(const FreeusdStage* stage);

/**
 * Root layer @c defaultPrim name. On @ref FREEUSD_OK, @p *out_prim_name is malloc'd; free with @ref freeusd_string_free.
 * @ref FREEUSD_ERR_NOT_FOUND if unset.
 */
FREEUSD_C_API int freeusd_stage_get_default_prim_utf8(const FreeusdStage* stage, char** out_prim_name);

/**
 * List direct child prim paths under @p parent_prim_utf8 (union across composed layers; stable sort).
 * On @ref FREEUSD_OK, sets @p *out_paths to a malloc'd array of @p *out_count malloc'd UTF-8 strings.
 * Free with @ref freeusd_path_list_free.
 */
FREEUSD_C_API int freeusd_stage_list_child_paths(const FreeusdStage* stage, const char* parent_prim_utf8,
                                                  char*** out_paths, size_t* out_count);

FREEUSD_C_API void freeusd_path_list_free(char** paths, size_t count);

/** Free @p values from @ref freeusd_stage_list_field_sample_times (single malloc block). */
FREEUSD_C_API void freeusd_double_array_free(double* values);

/**
 * Concatenated relationship targets (strongest layer first, then next layer’s targets, etc.).
 * On @ref FREEUSD_OK, @p *out_paths / @p *out_count follow @ref freeusd_stage_list_child_paths ownership.
 */
FREEUSD_C_API int freeusd_stage_list_relationship_targets(const FreeusdStage* stage, const char* prim_path_utf8,
                                                          const char* rel_name_utf8, char*** out_paths,
                                                          size_t* out_count);

/** 1 if any composed layer authors @p rel_name_utf8 on @p prim_path_utf8, 0 if not, negative @ref FreeusdResult on error. */
FREEUSD_C_API int freeusd_stage_has_relationship(const FreeusdStage* stage, const char* prim_path_utf8,
                                                 const char* rel_name_utf8);

/** 1 iff any composed layer authors a layer @c relocates entry with source @p from_prim_utf8, 0 if not, negative on error. */
FREEUSD_C_API int freeusd_stage_relocate_source_in_any_layer(const FreeusdStage* stage, const char* from_prim_utf8);

/**
 * Composed relocate target for source @p from_prim_utf8 (strongest layer wins).
 * On @ref FREEUSD_OK, @p *out_target_utf8 is malloc'd absolute prim path; free with @ref freeusd_string_free.
 * @ref FREEUSD_ERR_NOT_FOUND if no relocate maps that source.
 */
FREEUSD_C_API int freeusd_stage_get_composed_relocate_target_utf8(const FreeusdStage* stage, const char* from_prim_utf8,
                                                                   char** out_target_utf8);

/**
 * Sorted composed relocate pairs (see @ref freeusd::usd::Stage::ListComposedRelocates).
 * Each string is @c "FROM" + U+001F + @c "TO" (UTF-8 prim paths). On @ref FREEUSD_OK, @p *out_strings / @p *out_count
 * use @ref freeusd_path_list_free.
 */
FREEUSD_C_API int freeusd_stage_list_composed_relocate_pairs_utf8(const FreeusdStage* stage, char*** out_strings,
                                                                  size_t* out_count);

/** 1 iff any composed layer authors @c prefixSubstitutions for @p from_prefix_utf8, 0 if not, negative on error. */
FREEUSD_C_API int freeusd_stage_prefix_substitution_key_in_any_layer(const FreeusdStage* stage,
                                                                       const char* from_prefix_utf8);

/**
 * Composed @c prefixSubstitutions target for @p from_prefix_utf8 (strongest layer wins).
 * On @ref FREEUSD_OK, @p *out_to_prefix_utf8 is malloc'd UTF-8; free with @ref freeusd_string_free.
 * @ref FREEUSD_ERR_NOT_FOUND if unmapped; @ref FREEUSD_ERR_INVALID_ARGUMENT if @p from_prefix_utf8 is empty.
 */
FREEUSD_C_API int freeusd_stage_get_composed_prefix_substitution_utf8(const FreeusdStage* stage,
                                                                      const char* from_prefix_utf8,
                                                                      char** out_to_prefix_utf8);

/**
 * Sorted composed @c prefixSubstitutions pairs (see @ref freeusd::usd::Stage::ListComposedPrefixSubstitutions).
 * Each string is @c "FROM" + U+001F + @c "TO" (UTF-8 path prefixes). On @ref FREEUSD_OK, @p *out_strings / @p *out_count
 * use @ref freeusd_path_list_free.
 */
FREEUSD_C_API int freeusd_stage_list_composed_prefix_substitution_pairs_utf8(const FreeusdStage* stage,
                                                                             char*** out_strings, size_t* out_count);

/**
 * Concatenated prim @c references (strongest layer first). Each string is canonical authored form
 * (@c \@asset\@ optional @c \<\/PrimPath\> tail), matching @c PrimReference::FormatAuthoredForUsda.
 * On @ref FREEUSD_OK, @p *out_strings / @p *out_count use @ref freeusd_path_list_free.
 */
FREEUSD_C_API int freeusd_stage_list_prim_references(const FreeusdStage* stage, const char* prim_path_utf8,
                                                     char*** out_strings, size_t* out_count);

/** 1 if @ref freeusd_stage_list_prim_references would return a non-empty list, 0 if empty, negative @ref FreeusdResult on error. */
FREEUSD_C_API int freeusd_stage_has_prim_references(const FreeusdStage* stage, const char* prim_path_utf8);

/**
 * Concatenated @c inherits targets (absolute prim paths as UTF-8, strongest layer first).
 * On @ref FREEUSD_OK, @p *out_paths / @p *out_count use @ref freeusd_path_list_free.
 */
FREEUSD_C_API int freeusd_stage_list_prim_inherits(const FreeusdStage* stage, const char* prim_path_utf8,
                                                   char*** out_paths, size_t* out_count);

/** 1 if @ref freeusd_stage_list_prim_inherits would return a non-empty list, 0 if empty, negative @ref FreeusdResult on error. */
FREEUSD_C_API int freeusd_stage_has_prim_inherits(const FreeusdStage* stage, const char* prim_path_utf8);

/**
 * Concatenated @c specializes targets (absolute prim paths as UTF-8, strongest layer first).
 * On @ref FREEUSD_OK, @p *out_paths / @p *out_count use @ref freeusd_path_list_free.
 */
FREEUSD_C_API int freeusd_stage_list_prim_specializes(const FreeusdStage* stage, const char* prim_path_utf8,
                                                      char*** out_paths, size_t* out_count);

/** 1 if @ref freeusd_stage_list_prim_specializes would return a non-empty list, 0 if empty, negative @ref FreeusdResult on error. */
FREEUSD_C_API int freeusd_stage_has_prim_specializes(const FreeusdStage* stage, const char* prim_path_utf8);

/**
 * Concatenated @c payload entries (strongest layer first; same string encoding as @ref freeusd_stage_list_prim_references).
 * On @ref FREEUSD_OK, @p *out_strings / @p *out_count use @ref freeusd_path_list_free.
 */
FREEUSD_C_API int freeusd_stage_list_prim_payloads(const FreeusdStage* stage, const char* prim_path_utf8,
                                                   char*** out_strings, size_t* out_count);

/** 1 if @ref freeusd_stage_list_prim_payloads would return a non-empty list, 0 if empty, negative @ref FreeusdResult on error. */
FREEUSD_C_API int freeusd_stage_has_prim_payloads(const FreeusdStage* stage, const char* prim_path_utf8);

/**
 * Sorted union of authored attribute field names on @p prim_path_utf8 across composed layers.
 * On @ref FREEUSD_OK, @p *out_names / @p *out_count use @ref freeusd_path_list_free.
 */
FREEUSD_C_API int freeusd_stage_list_composed_field_names(const FreeusdStage* stage, const char* prim_path_utf8,
                                                          char*** out_names, size_t* out_count);

/**
 * Sorted union of relationship names on @p prim_path_utf8 across composed layers.
 * On @ref FREEUSD_OK, @p *out_names / @p *out_count use @ref freeusd_path_list_free.
 */
FREEUSD_C_API int freeusd_stage_list_composed_relationship_names(const FreeusdStage* stage,
                                                                 const char* prim_path_utf8, char*** out_names,
                                                                 size_t* out_count);

/** 1 if prim path exists on the composed stage, 0 if not, negative @ref FreeusdResult on error. */
FREEUSD_C_API int freeusd_stage_prim_is_valid(const FreeusdStage* stage, const char* prim_path_utf8);

/**
 * Read evaluated attribute as double at @p time (strongest opinion; follows
 * attribute connections where implemented).
 * @return @ref FREEUSD_OK or error code; @p out_value unchanged on failure.
 */
FREEUSD_C_API int freeusd_stage_read_field_double(const FreeusdStage* stage, const char* prim_path_utf8,
                                                  const char* attr_name_utf8, double time, double* out_value);

/**
 * Sorted union of authored time-sample times for @p attr_name_utf8 on @p prim_path_utf8 across composed layers.
 * On @ref FREEUSD_OK, @p *out_times points to @p *out_count doubles (single malloc block); free with @ref freeusd_double_array_free.
 */
FREEUSD_C_API int freeusd_stage_list_field_sample_times(const FreeusdStage* stage, const char* prim_path_utf8,
                                                        const char* attr_name_utf8, double** out_times,
                                                        size_t* out_count);

/** 1 if any composed layer authors @p attr_name_utf8 on @p prim_path_utf8, 0 if not, negative @ref FreeusdResult on error. */
FREEUSD_C_API int freeusd_stage_has_field_opinion(const FreeusdStage* stage, const char* prim_path_utf8,
                                                  const char* attr_name_utf8);

/** 1 if any composed layer authors @c attr.connect for @p attr_name_utf8, 0 if not, negative @ref FreeusdResult on error. */
FREEUSD_C_API int freeusd_stage_has_attribute_connection(const FreeusdStage* stage, const char* prim_path_utf8,
                                                         const char* attr_name_utf8);

/**
 * Strongest composed @c .connect target for @p attr_name_utf8 (full property path as UTF-8).
 * On @ref FREEUSD_OK, @p *out_target_utf8 is malloc'd; free with @ref freeusd_string_free.
 */
FREEUSD_C_API int freeusd_stage_get_attribute_connection_target(const FreeusdStage* stage,
                                                                const char* prim_path_utf8,
                                                                const char* attr_name_utf8, char** out_target_utf8);

/**
 * Read evaluated attribute as bool at @p time (payload must be bool).
 * @p out_value receives 0 or 1.
 */
FREEUSD_C_API int freeusd_stage_read_field_bool(const FreeusdStage* stage, const char* prim_path_utf8,
                                                const char* attr_name_utf8, double time, int* out_value);

/**
 * Read evaluated attribute as integer at @p time (bool coerces to 0/1; int32/int64/float/double truncated toward zero).
 */
FREEUSD_C_API int freeusd_stage_read_field_int64(const FreeusdStage* stage, const char* prim_path_utf8,
                                                 const char* attr_name_utf8, double time, int64_t* out_value);

/**
 * Read evaluated attribute default/string at @p time (string payload only; fails if not a string).
 * On @ref FREEUSD_OK, @p *out_string is malloc'd UTF-8; free with @ref freeusd_string_free.
 */
FREEUSD_C_API int freeusd_stage_read_field_string(const FreeusdStage* stage, const char* prim_path_utf8,
                                                  const char* attr_name_utf8, double time, char** out_string);

/**
 * Read evaluated attribute as @c double3 / Vec3d at @p time. All @p out_* pointers must be non-NULL.
 */
FREEUSD_C_API int freeusd_stage_read_field_vec3d(const FreeusdStage* stage, const char* prim_path_utf8,
                                                 const char* attr_name_utf8, double time, double* out_x,
                                                 double* out_y, double* out_z);

/**
 * Composed prim active flag (strongest opinion; default true if no opinion).
 * @p out_active receives 0 or 1.
 */
FREEUSD_C_API int freeusd_stage_resolve_prim_active(const FreeusdStage* stage, const char* prim_path_utf8,
                                                    int* out_active);

/** 1 if any composed layer authors @c active on the prim, 0 if not, negative @ref FreeusdResult on error. */
FREEUSD_C_API int freeusd_stage_resolve_has_prim_active_opinion(const FreeusdStage* stage,
                                                                const char* prim_path_utf8);

/** Composed USDA prim @c def / @c class / @c over marker (strongest authored non-@c def opinion wins). */
enum FreeusdPrimSpecifierKind {
  FREEUSD_PRIM_SPECIFIER_DEFAULT = 0,
  FREEUSD_PRIM_SPECIFIER_DEF = 1,
  FREEUSD_PRIM_SPECIFIER_CLASS = 2,
  FREEUSD_PRIM_SPECIFIER_OVER = 3,
};

/**
 * Composed prim specifier kind for @p prim_path_utf8 (see @ref FreeusdPrimSpecifierKind).
 * On success returns @c 0–@c 3. On error returns the negation of a @ref FreeusdResult code (e.g. @c -1 invalid argument).
 */
FREEUSD_C_API int freeusd_stage_resolve_prim_specifier_kind(const FreeusdStage* stage, const char* prim_path_utf8);

/**
 * Composed prim schema kind (e.g. "Xform"). Returns malloc'd UTF-8 or NULL if none / error.
 * Free with @ref freeusd_string_free.
 */
FREEUSD_C_API char* freeusd_stage_resolve_prim_kind(const FreeusdStage* stage, const char* prim_path_utf8);

/** 1 if composed prim has a schema kind opinion, 0 if not, negative @ref FreeusdResult on error. */
FREEUSD_C_API int freeusd_stage_resolve_has_prim_kind(const FreeusdStage* stage, const char* prim_path_utf8);

/**
 * Composed @c customData entry for @p key (strongest layer wins). On @ref FREEUSD_OK, @p *out_value
 * is malloc'd UTF-8 (from string-typed stored values). Free with @ref freeusd_string_free.
 */
FREEUSD_C_API int freeusd_stage_get_composed_prim_custom_data(const FreeusdStage* stage, const char* prim_path_utf8,
                                                                const char* key_utf8, char** out_value);

/** 1 if any composed layer authors @p key_utf8 in prim @c customData, 0 if not, negative @ref FreeusdResult on error. */
FREEUSD_C_API int freeusd_stage_prim_custom_data_key_in_any_layer(const FreeusdStage* stage,
                                                                   const char* prim_path_utf8,
                                                                   const char* key_utf8);

/**
 * Sorted union of @c customData keys for @p prim_path_utf8 across composed layers.
 * On @ref FREEUSD_OK, @p *out_keys / @p *out_count follow @ref freeusd_stage_list_child_paths ownership.
 */
FREEUSD_C_API int freeusd_stage_list_composed_prim_custom_data_keys(const FreeusdStage* stage,
                                                                    const char* prim_path_utf8, char*** out_keys,
                                                                    size_t* out_count);

/**
 * Composed root @c customLayerData entry for @p key_utf8 (strongest layer wins).
 * On @ref FREEUSD_OK, @p *out_value is malloc'd UTF-8 for string- or token-typed values only; free with @ref freeusd_string_free.
 * @ref FREEUSD_ERR_NOT_FOUND if unset or value is not string/token (use C++ for other types).
 * @ref FREEUSD_ERR_INVALID_ARGUMENT if @p key_utf8 is empty.
 */
FREEUSD_C_API int freeusd_stage_get_composed_custom_layer_data_utf8(const FreeusdStage* stage, const char* key_utf8,
                                                                    char** out_value);

/** 1 if any composed layer authors @p key_utf8 in @c customLayerData, 0 if not, negative @ref FreeusdResult on error. */
FREEUSD_C_API int freeusd_stage_custom_layer_data_key_in_any_layer(const FreeusdStage* stage, const char* key_utf8);

/**
 * Sorted union of @c customLayerData keys across the composed layer stack.
 * On @ref FREEUSD_OK, @p *out_keys / @p *out_count follow @ref freeusd_stage_list_child_paths ownership.
 */
FREEUSD_C_API int freeusd_stage_list_composed_custom_layer_data_keys(const FreeusdStage* stage, char*** out_keys,
                                                                     size_t* out_count);

/**
 * Composed @c variantSelection name for @p variant_set_utf8 on @p prim_path_utf8 (strongest layer wins).
 * On @ref FREEUSD_OK, @p *out_selected_utf8 is malloc'd UTF-8; free with @ref freeusd_string_free.
 * @ref FREEUSD_ERR_NOT_FOUND if unset.
 * @ref FREEUSD_ERR_INVALID_ARGUMENT if @p variant_set_utf8 is empty or @p prim_path_utf8 is not a valid prim path.
 */
FREEUSD_C_API int freeusd_stage_get_composed_prim_variant_selection_utf8(const FreeusdStage* stage,
                                                                         const char* prim_path_utf8,
                                                                         const char* variant_set_utf8,
                                                                         char** out_selected_utf8);

/** 1 if any composed layer authors @p variant_set_utf8 in @c variantSelection on @p prim_path_utf8, 0 if not. */
FREEUSD_C_API int freeusd_stage_prim_variant_selection_set_in_any_layer(const FreeusdStage* stage,
                                                                          const char* prim_path_utf8,
                                                                          const char* variant_set_utf8);

/**
 * Sorted union of @c variantSelection set names on @p prim_path_utf8 across composed layers.
 * On @ref FREEUSD_OK, @p *out_strings / @p *out_count use @ref freeusd_path_list_free.
 */
FREEUSD_C_API int freeusd_stage_list_composed_prim_variant_selection_sets_utf8(const FreeusdStage* stage,
                                                                                const char* prim_path_utf8,
                                                                                char*** out_strings, size_t* out_count);

/**
 * Sorted union of @c variantSets keys declared on @p prim_path_utf8 across composed layers.
 * On @ref FREEUSD_OK, @p *out_strings / @p *out_count use @ref freeusd_path_list_free.
 */
FREEUSD_C_API int freeusd_stage_list_composed_prim_variant_set_names_utf8(const FreeusdStage* stage,
                                                                            const char* prim_path_utf8,
                                                                            char*** out_strings, size_t* out_count);

/** 1 if any composed layer declares @p variant_set_name_utf8 in @c variantSets on @p prim_path_utf8, 0 if not. */
FREEUSD_C_API int freeusd_stage_prim_variant_set_declared_in_any_layer(const FreeusdStage* stage,
                                                                       const char* prim_path_utf8,
                                                                       const char* variant_set_name_utf8);

/**
 * Variant names for @p variant_set_name_utf8 from the strongest composed layer that declares that set (authored order).
 * On @ref FREEUSD_OK, @p *out_strings / @p *out_count use @ref freeusd_path_list_free.
 * @ref FREEUSD_ERR_NOT_FOUND if the set is not declared on any composed layer (or @p variant_set_name_utf8 is empty).
 */
FREEUSD_C_API int freeusd_stage_list_composed_prim_variant_names_utf8(const FreeusdStage* stage,
                                                                      const char* prim_path_utf8,
                                                                      const char* variant_set_name_utf8,
                                                                      char*** out_strings, size_t* out_count);

#ifdef __cplusplus
}
#endif

#endif /* FREEUSD_C_FREEUSD_H */
