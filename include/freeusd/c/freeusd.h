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

/** USDC crate file magic / identifier prefix (``PXR-USDC``; static storage; do not free). */
FREEUSD_C_API const char* freeusd_usdc_crate_identifier_utf8(void);

/**
 * Sniff-only classification of a file on disk (reads a prefix; no full ``.usdc`` decode).
 * Values match @c freeusd::usd::crate::UsdFileKind.
 */
enum FreeusdUsdFileKind {
  FREEUSD_USD_FILE_IO_OR_EMPTY = 0,
  FREEUSD_USD_FILE_USDA_ASCII = 1,
  FREEUSD_USD_FILE_USDC_CRATE = 2,
  FREEUSD_USD_FILE_UNKNOWN = 3,
};

/**
 * Reads the first bytes of @p path_utf8. On @ref FREEUSD_OK, @p *out_kind is set to @ref FreeusdUsdFileKind.
 * When the kind is @ref FREEUSD_USD_FILE_IO_OR_EMPTY and diagnostics were recorded, @p *out_detail_utf8 may be a
 * malloc'd UTF-8 string (free with @ref freeusd_string_free); otherwise @p *out_detail_utf8 is NULL.
 * Pass @c NULL for @p out_detail_utf8 if details are not needed.
 */
FREEUSD_C_API int freeusd_detect_usd_file_kind_from_path_utf8(const char* path_utf8, int* out_kind,
                                                               char** out_detail_utf8);

/** Fixed-size USDC crate bootstrap (``ReadUsdCrateBootstrapFromPath``); matches C++ logical fields. */
typedef struct FreeusdUsdcBootstrap {
  uint8_t file_version_major;
  uint8_t file_version_minor;
  uint8_t file_version_patch;
  uint8_t reserved[5]; /**< Pad so @c toc_byte_offset is 8-byte aligned (portable C layout). */
  int64_t toc_byte_offset;
} FreeusdUsdcBootstrap;

#if defined(__cplusplus)
static_assert(sizeof(FreeusdUsdcBootstrap) == 16u, "FreeusdUsdcBootstrap must be 16 bytes (FFI)");
#elif defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
_Static_assert(sizeof(FreeusdUsdcBootstrap) == 16u, "FreeusdUsdcBootstrap must be 16 bytes (FFI)");
#endif

/**
 * Reads the 88-byte USDC bootstrap from @p path_utf8 (little-endian TOC offset). On @ref FREEUSD_OK,
 * @p *out_bootstrap is filled; otherwise it is zeroed and @ref freeusd_last_error_message describes the failure.
 */
FREEUSD_C_API int freeusd_read_usdc_bootstrap_from_path_utf8(const char* path_utf8,
                                                              FreeusdUsdcBootstrap* out_bootstrap);

/** One USDC TOC section record on disk (``ReadUsdCrateTocFromPath``); **32** bytes per entry, little-endian ints. */
typedef struct FreeusdUsdcTocSection {
  char name[16]; /**< NUL-terminated or NUL-padded section key (at most 15 chars + NUL). */
  int64_t start_byte_offset;
  int64_t size_bytes;
} FreeusdUsdcTocSection;

#if defined(__cplusplus)
static_assert(sizeof(FreeusdUsdcTocSection) == 32u, "FreeusdUsdcTocSection must be 32 bytes (FFI)");
#elif defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
_Static_assert(sizeof(FreeusdUsdcTocSection) == 32u, "FreeusdUsdcTocSection must be 32 bytes (FFI)");
#endif

/**
 * Reads the USDC table of contents after the bootstrap. On @ref FREEUSD_OK, @p *out_total_section_count is the
 * count from the file, @p *out_sections_returned equals that count, and @p *out_sections points to a malloc'd
 * array of @p *out_sections_returned elements (or NULL when the count is zero). Free with @ref freeusd_usdc_toc_sections_free.
 * Fails with @ref FREEUSD_ERR_INVALID_ARGUMENT if @p max_sections is zero, or @ref FREEUSD_ERR_PARSE if the file
 * count exceeds @p max_sections or the file is too short.
 */
FREEUSD_C_API int freeusd_read_usdc_toc_from_path_utf8(const char* path_utf8, uint64_t max_sections,
                                                       uint64_t* out_total_section_count,
                                                       FreeusdUsdcTocSection** out_sections,
                                                       uint64_t* out_sections_returned);

/** Frees @p sections from @ref freeusd_read_usdc_toc_from_path_utf8 (safe on NULL). */
FREEUSD_C_API void freeusd_usdc_toc_sections_free(FreeusdUsdcTocSection* sections);

/**
 * Reads one USDC TOC section payload by name. On @ref FREEUSD_OK, @p *out_bytes points to a malloc'd byte buffer of
 * @p *out_size bytes (or NULL when the section exists but is empty). Free the allocation with @ref freeusd_bytes_free.
 * Fails with @ref FREEUSD_ERR_NOT_FOUND if the section is absent, @ref FREEUSD_ERR_INVALID_ARGUMENT for null pointers
 * / empty names, or @ref FREEUSD_ERR_PARSE when the payload exceeds @p max_bytes or cannot be read fully.
 */
FREEUSD_C_API int freeusd_read_usdc_section_bytes_from_path_utf8(const char* path_utf8, const char* section_name_utf8,
                                                                 uint64_t max_bytes, uint8_t** out_bytes,
                                                                 uint64_t* out_size);

/** Frees byte buffers returned by @ref freeusd_read_usdc_section_bytes_from_path_utf8 (safe on NULL). */
FREEUSD_C_API void freeusd_bytes_free(void* bytes);

/**
 * Reads the validated ``TOKENS`` table payload from a shared test fixture style ``.usdc`` file.
 * On @ref FREEUSD_OK, @p *out_strings / @p *out_count use @ref freeusd_path_list_free.
 */
FREEUSD_C_API int freeusd_read_usdc_token_table_from_path_utf8(const char* path_utf8, uint64_t max_entries,
                                                               uint64_t max_total_bytes, char*** out_strings,
                                                               size_t* out_count);

/**
 * Reads the validated ``STRINGS`` table payload from a shared test fixture style ``.usdc`` file.
 * On @ref FREEUSD_OK, @p *out_strings / @p *out_count use @ref freeusd_path_list_free.
 */
FREEUSD_C_API int freeusd_read_usdc_string_table_from_path_utf8(const char* path_utf8, uint64_t max_entries,
                                                                uint64_t max_total_bytes, char*** out_strings,
                                                                size_t* out_count);

/**
 * Reads the validated ``PATHS`` table payload from a shared test fixture style ``.usdc`` file.
 * On @ref FREEUSD_OK, @p *out_paths / @p *out_count use @ref freeusd_path_list_free.
 */
FREEUSD_C_API int freeusd_read_usdc_path_table_from_path_utf8(const char* path_utf8, uint64_t max_entries,
                                                              uint64_t max_total_bytes, char*** out_paths,
                                                              size_t* out_count);

/** One row from the validated fixture-oriented ``FIELDS`` table (``ReadUsdCrateFieldsTableFromPath``). */
typedef struct FreeusdUsdcFieldEntry {
  uint64_t token_index;
  uint64_t value_type_token_index;
} FreeusdUsdcFieldEntry;

#if defined(__cplusplus)
static_assert(sizeof(FreeusdUsdcFieldEntry) == 16u, "FreeusdUsdcFieldEntry must be 16 bytes (FFI)");
#elif defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
_Static_assert(sizeof(FreeusdUsdcFieldEntry) == 16u, "FreeusdUsdcFieldEntry must be 16 bytes (FFI)");
#endif

/**
 * Reads the validated ``FIELDS`` table payload from a shared test fixture style ``.usdc`` file.
 * On @ref FREEUSD_OK, @p *out_entries / @p *out_count use @ref freeusd_usdc_fields_entries_free.
 */
FREEUSD_C_API int freeusd_read_usdc_fields_table_from_path_utf8(const char* path_utf8, uint64_t max_entries,
                                                                uint64_t max_total_bytes,
                                                                FreeusdUsdcFieldEntry** out_entries, size_t* out_count);

/** Frees @p entries from @ref freeusd_read_usdc_fields_table_from_path_utf8 (safe on NULL). */
FREEUSD_C_API void freeusd_usdc_fields_entries_free(FreeusdUsdcFieldEntry* entries);

/** One row from the validated fixture-oriented ``SPECS`` table (``ReadUsdCrateSpecsTableFromPath``). */
typedef struct FreeusdUsdcSpecEntry {
  uint64_t path_index;
  uint64_t field_set_index;
  uint64_t spec_type;
} FreeusdUsdcSpecEntry;

#if defined(__cplusplus)
static_assert(sizeof(FreeusdUsdcSpecEntry) == 24u, "FreeusdUsdcSpecEntry must be 24 bytes (FFI)");
#elif defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
_Static_assert(sizeof(FreeusdUsdcSpecEntry) == 24u, "FreeusdUsdcSpecEntry must be 24 bytes (FFI)");
#endif

/**
 * Reads the validated ``SPECS`` table payload from a shared test fixture style ``.usdc`` file.
 * On @ref FREEUSD_OK, @p *out_entries / @p *out_count use @ref freeusd_usdc_specs_entries_free.
 */
FREEUSD_C_API int freeusd_read_usdc_specs_table_from_path_utf8(const char* path_utf8, uint64_t max_entries,
                                                               uint64_t max_total_bytes,
                                                               FreeusdUsdcSpecEntry** out_entries, size_t* out_count);

/** Frees @p entries from @ref freeusd_read_usdc_specs_table_from_path_utf8 (safe on NULL). */
FREEUSD_C_API void freeusd_usdc_specs_entries_free(FreeusdUsdcSpecEntry* entries);

/** One field set from the validated fixture-oriented ``FIELDSETS`` table. */
typedef struct FreeusdUsdcFieldSet {
  uint64_t field_count;
  uint64_t* field_indices;
} FreeusdUsdcFieldSet;

/**
 * Reads the validated ``FIELDSETS`` table payload from a shared test fixture style ``.usdc`` file.
 * On @ref FREEUSD_OK, @p *out_sets / @p *out_count use @ref freeusd_usdc_fieldsets_free.
 */
FREEUSD_C_API int freeusd_read_usdc_fieldsets_table_from_path_utf8(const char* path_utf8, uint64_t max_field_sets,
                                                                   uint64_t max_fields_per_set,
                                                                   uint64_t max_total_bytes,
                                                                   FreeusdUsdcFieldSet** out_sets, size_t* out_count);

/** Frees @p sets from @ref freeusd_read_usdc_fieldsets_table_from_path_utf8 (safe on NULL). */
FREEUSD_C_API void freeusd_usdc_fieldsets_free(FreeusdUsdcFieldSet* sets, size_t count);

/** One opaque value blob from the validated fixture-oriented ``VALUES`` table. */
typedef struct FreeusdUsdcValueBlob {
  uint64_t byte_count;
  uint8_t* bytes;
} FreeusdUsdcValueBlob;

/**
 * Reads the validated ``VALUES`` table payload from a shared test fixture style ``.usdc`` file.
 * On @ref FREEUSD_OK, @p *out_blobs / @p *out_count use @ref freeusd_usdc_values_blobs_free.
 */
FREEUSD_C_API int freeusd_read_usdc_values_table_from_path_utf8(const char* path_utf8, uint64_t max_entries,
                                                              uint64_t max_total_bytes,
                                                              FreeusdUsdcValueBlob** out_blobs, size_t* out_count);

/** Frees @p blobs from @ref freeusd_read_usdc_values_table_from_path_utf8 (safe on NULL). */
FREEUSD_C_API void freeusd_usdc_values_blobs_free(FreeusdUsdcValueBlob* blobs, size_t count);

/** Fixture-oriented typed value kinds in the ``VALUES`` table. */
typedef enum FreeusdUsdcTypedValueKind {
  FREEUSD_USDC_VALUE_OPAQUE = 0,
  FREEUSD_USDC_VALUE_INT32 = 1,
  FREEUSD_USDC_VALUE_FLOAT = 2,
  FREEUSD_USDC_VALUE_TOKEN_INDEX = 3,
  FREEUSD_USDC_VALUE_BOOL = 4,
  FREEUSD_USDC_VALUE_DOUBLE = 5,
  FREEUSD_USDC_VALUE_INT64 = 6,
  FREEUSD_USDC_VALUE_STRING_UTF8 = 7,
  FREEUSD_USDC_VALUE_VEC3F = 8,
  FREEUSD_USDC_VALUE_STRING_INDEX = 9,
  FREEUSD_USDC_VALUE_VEC3D = 10,
  FREEUSD_USDC_VALUE_INT32_ARRAY = 11,
  FREEUSD_USDC_VALUE_FLOAT_ARRAY = 12,
  FREEUSD_USDC_VALUE_DOUBLE_ARRAY = 13,
  FREEUSD_USDC_VALUE_VEC2F = 14,
  FREEUSD_USDC_VALUE_VEC4F = 15,
} FreeusdUsdcTypedValueKind;

typedef struct FreeusdUsdcTypedValue {
  uint64_t kind;
  uint64_t byte_count;
  uint8_t* bytes;
  int32_t int32_value;
  float float_value;
  uint64_t token_index;
  int bool_value;
  double double_value;
  int64_t int64_value;
  char* string_utf8;
  float vec3f_value[3];
  uint64_t string_index;
  double vec3d_value[3];
  int32_t* int32_array;
  size_t int32_array_count;
  float* float_array;
  size_t float_array_count;
  double* double_array;
  size_t double_array_count;
  float vec2f_value[2];
  float vec4f_value[4];
} FreeusdUsdcTypedValue;

FREEUSD_C_API int freeusd_read_usdc_typed_values_table_from_path_utf8(const char* path_utf8, uint64_t max_entries,
                                                                      uint64_t max_total_bytes,
                                                                      FreeusdUsdcTypedValue** out_values,
                                                                      size_t* out_count);

FREEUSD_C_API void freeusd_usdc_typed_values_free(FreeusdUsdcTypedValue* values, size_t count);

/** Reads the embedded ``USDA`` crate section as UTF-8 text (fixture-oriented bridge). */
FREEUSD_C_API int freeusd_read_usdc_usda_section_from_path_utf8(const char* path_utf8, uint64_t max_text_bytes,
                                                              char** out_text_utf8);

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
 * Compute the prim's local transform using the validated `usdGeom::Xformable` subset.
 * On @ref FREEUSD_OK, @p out_row_major receives 16 doubles in row-major order.
 * Returns @ref FREEUSD_ERR_NOT_FOUND for invalid prims.
 */
FREEUSD_C_API int freeusd_stage_compute_local_transform_matrix4d(const FreeusdStage* stage, const char* prim_path_utf8,
                                                                 double time, double* out_row_major);

/**
 * Compute the prim's local-to-world transform using the validated `usdGeom::Xformable` subset.
 * On @ref FREEUSD_OK, @p out_row_major receives 16 doubles in row-major order.
 * Returns @ref FREEUSD_ERR_NOT_FOUND for invalid prims.
 */
FREEUSD_C_API int freeusd_stage_compute_local_to_world_transform_matrix4d(const FreeusdStage* stage,
                                                                          const char* prim_path_utf8, double time,
                                                                          double* out_row_major);

/**
 * Compute inherited `visibility` using the validated `usdGeom::Imageable` subset.
 * On @ref FREEUSD_OK, @p out_visible receives 0 or 1.
 * Returns @ref FREEUSD_ERR_NOT_FOUND for invalid prims.
 */
FREEUSD_C_API int freeusd_stage_compute_imageable_visibility(const FreeusdStage* stage, const char* prim_path_utf8,
                                                             double time, int* out_visible);

/**
 * Compute inherited `purpose` using the validated `usdGeom::Imageable` subset.
 * On @ref FREEUSD_OK, @p *out_purpose_utf8 is malloc'd UTF-8; free with @ref freeusd_string_free.
 * Returns @ref FREEUSD_ERR_NOT_FOUND for invalid prims.
 */
FREEUSD_C_API int freeusd_stage_compute_imageable_purpose_utf8(const FreeusdStage* stage, const char* prim_path_utf8,
                                                               double time, char** out_purpose_utf8);

/**
 * Compute the prim's local-space bounds using the validated `usdGeom::Boundable` subset.
 * On @ref FREEUSD_OK, @p out_min_* and @p out_max_* receive the box corners. Empty boxes are reported as
 * @ref FREEUSD_ERR_NOT_FOUND.
 */
FREEUSD_C_API int freeusd_stage_compute_boundable_local_bounds(const FreeusdStage* stage, const char* prim_path_utf8,
                                                               double time, double* out_min_x, double* out_min_y,
                                                               double* out_min_z, double* out_max_x, double* out_max_y,
                                                               double* out_max_z);

/**
 * Compute the prim's world-space bounds using the validated `usdGeom::Boundable` subset.
 * On @ref FREEUSD_OK, @p out_min_* and @p out_max_* receive the box corners. Empty boxes are reported as
 * @ref FREEUSD_ERR_NOT_FOUND.
 */
FREEUSD_C_API int freeusd_stage_compute_boundable_world_bounds(const FreeusdStage* stage, const char* prim_path_utf8,
                                                               double time, double* out_min_x, double* out_min_y,
                                                               double* out_min_z, double* out_max_x, double* out_max_y,
                                                               double* out_max_z);

/**
 * Read evaluated attribute as double at @p time (strongest opinion; follows
 * attribute connections where implemented).
 * Succeeds for @c double payloads, or @c float / @c int32 / @c int64 coerced to @c double.
 * Missing prims, missing attributes, and unsupported coercions report @ref FREEUSD_ERR_NOT_FOUND.
 * @return @ref FREEUSD_OK or error code; @p out_value unchanged on failure.
 */
FREEUSD_C_API int freeusd_stage_read_field_double(const FreeusdStage* stage, const char* prim_path_utf8,
                                                  const char* attr_name_utf8, double time, double* out_value);

/**
 * Read evaluated attribute as @c float at @p time (strongest opinion; follows connections where implemented).
 * Succeeds for @c float payloads, or @c double / @c int32 / @c int64 / @c bool coerced to @c float.
 * Missing prims, missing attributes, and unsupported coercions report @ref FREEUSD_ERR_NOT_FOUND.
 */
FREEUSD_C_API int freeusd_stage_read_field_float(const FreeusdStage* stage, const char* prim_path_utf8,
                                                 const char* attr_name_utf8, double time, float* out_value);

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
 * Missing prims, missing attributes, and non-bool payloads report @ref FREEUSD_ERR_NOT_FOUND.
 * @p out_value receives 0 or 1.
 */
FREEUSD_C_API int freeusd_stage_read_field_bool(const FreeusdStage* stage, const char* prim_path_utf8,
                                                const char* attr_name_utf8, double time, int* out_value);

/**
 * Read evaluated attribute as integer at @p time (bool coerces to 0/1; int32/int64/float/double truncated toward zero).
 * Missing prims, missing attributes, and unsupported coercions report @ref FREEUSD_ERR_NOT_FOUND.
 */
FREEUSD_C_API int freeusd_stage_read_field_int64(const FreeusdStage* stage, const char* prim_path_utf8,
                                                 const char* attr_name_utf8, double time, int64_t* out_value);

/**
 * Read evaluated attribute as UTF-8 string at @p time (string payload, or token text).
 * Missing prims, missing attributes, and non-string/non-token payloads report @ref FREEUSD_ERR_NOT_FOUND.
 * On @ref FREEUSD_OK, @p *out_string is malloc'd UTF-8; free with @ref freeusd_string_free.
 */
FREEUSD_C_API int freeusd_stage_read_field_string(const FreeusdStage* stage, const char* prim_path_utf8,
                                                  const char* attr_name_utf8, double time, char** out_string);

/**
 * Read evaluated attribute as @c double3 / Vec3d at @p time. All @p out_* pointers must be non-NULL.
 * Missing prims, missing attributes, and non-@c Vec3d payloads report @ref FREEUSD_ERR_NOT_FOUND.
 */
FREEUSD_C_API int freeusd_stage_read_field_vec3d(const FreeusdStage* stage, const char* prim_path_utf8,
                                                 const char* attr_name_utf8, double time, double* out_x,
                                                 double* out_y, double* out_z);

/**
 * Read evaluated attribute as @c float3 / Vec3f at @p time. All @p out_* pointers must be non-NULL.
 * Succeeds for @c Vec3f payloads, or @c Vec3d / @c double3 (components narrowed to @c float).
 * Missing prims, missing attributes, and unsupported coercions report @ref FREEUSD_ERR_NOT_FOUND.
 */
FREEUSD_C_API int freeusd_stage_read_field_vec3f(const FreeusdStage* stage, const char* prim_path_utf8,
                                                 const char* attr_name_utf8, double time, float* out_x,
                                                 float* out_y, float* out_z);

/**
 * Read evaluated @c matrix4d at @p time (strict; @c Matrix4d / @c double4x4 only).
 * Missing prims, missing attributes, and non-matrix payloads report @ref FREEUSD_ERR_NOT_FOUND.
 * @p out_row_major must point to 16 @c double values in row-major order @c m[row*4+col]
 * (row homogeneous vectors @c [x y z 1] * M).
 */
FREEUSD_C_API int freeusd_stage_read_field_matrix4d(const FreeusdStage* stage, const char* prim_path_utf8,
                                                    const char* attr_name_utf8, double time,
                                                    double* out_row_major);

/**
 * Read evaluated @c quatd at @p time (strict; @c quatd payload only). Components are @c (real, i, j, k).
 * Missing prims, missing attributes, and non-@c quatd payloads report @ref FREEUSD_ERR_NOT_FOUND.
 */
FREEUSD_C_API int freeusd_stage_read_field_quatd(const FreeusdStage* stage, const char* prim_path_utf8,
                                                 const char* attr_name_utf8, double time, double* out_real,
                                                 double* out_i, double* out_j, double* out_k);

/**
 * Read evaluated @c quatf at @p time (@c quatf payload, or @c quatd narrowed to @c float).
 * Missing prims, missing attributes, and unsupported coercions report @ref FREEUSD_ERR_NOT_FOUND.
 */
FREEUSD_C_API int freeusd_stage_read_field_quatf(const FreeusdStage* stage, const char* prim_path_utf8,
                                                 const char* attr_name_utf8, double time, float* out_real,
                                                 float* out_i, float* out_j, float* out_k);

/**
 * Read evaluated @c token at @p time (token payload only; fails for plain string).
 * Missing prims, missing attributes, and non-token payloads report @ref FREEUSD_ERR_NOT_FOUND.
 * On @ref FREEUSD_OK, @p *out_token_utf8 is malloc'd UTF-8; free with @ref freeusd_string_free.
 */
FREEUSD_C_API int freeusd_stage_read_field_token(const FreeusdStage* stage, const char* prim_path_utf8,
                                                 const char* attr_name_utf8, double time, char** out_token_utf8);

/**
 * Read evaluated @c token[] at @p time.
 * Missing prims, missing attributes, and non-@c token[] payloads report @ref FREEUSD_ERR_NOT_FOUND.
 * On @ref FREEUSD_OK, @p *out_strings / @p *out_count use @ref freeusd_path_list_free (possibly empty).
 */
FREEUSD_C_API int freeusd_stage_read_field_token_array(const FreeusdStage* stage, const char* prim_path_utf8,
                                                       const char* attr_name_utf8, double time, char*** out_strings,
                                                       size_t* out_count);

/**
 * Read ``joints`` token[] from a composed @c Skeleton prim.
 * On @ref FREEUSD_OK, @p *out_strings / @p *out_count use @ref freeusd_path_list_free.
 */
FREEUSD_C_API int freeusd_stage_read_skel_joint_names(const FreeusdStage* stage, const char* skeleton_path_utf8,
                                                      char*** out_strings, size_t* out_count);

/**
 * Resolve a Material ``outputs:surface`` connection to the connected shader prim path.
 * On @ref FREEUSD_OK, @p *out_shader_path_utf8 is malloc'd; free with @ref freeusd_string_free.
 */
FREEUSD_C_API int freeusd_stage_read_material_surface_shader_path(const FreeusdStage* stage,
                                                                  const char* material_path_utf8,
                                                                  char** out_shader_path_utf8);

/**
 * Read ``UsdPreviewSurface`` ``inputs:diffuseColor`` (color3f) at @p time from a shader prim.
 * @p out_rgb must point to three floats (r, g, b). Missing shader or input reports @ref FREEUSD_ERR_NOT_FOUND.
 */
FREEUSD_C_API int freeusd_stage_read_preview_surface_diffuse_color(const FreeusdStage* stage,
                                                                 const char* shader_path_utf8, double time,
                                                                 float out_rgb[3]);

/**
 * Read ``UsdPreviewSurface`` diffuse texture asset path (direct asset or connected ``inputs:file``).
 * On @ref FREEUSD_OK, @p *out_path_utf8 is malloc'd; free with @ref freeusd_string_free.
 */
FREEUSD_C_API int freeusd_stage_read_preview_surface_diffuse_texture_asset_path(
    const FreeusdStage* stage, const char* shader_path_utf8, double time, char** out_path_utf8);

/** Evaluated ``UsdLuxDistantLight`` scalar inputs at a time code. */
typedef struct FreeusdLuxDistantLightSample {
  float intensity;
  float color[3];
  float angle;
} FreeusdLuxDistantLightSample;

/**
 * Read ``UsdLuxDistantLight`` ``inputs:intensity``, ``inputs:color``, and ``inputs:angle`` at @p time.
 * Missing light prims or missing inputs report @ref FREEUSD_ERR_NOT_FOUND.
 */
FREEUSD_C_API int freeusd_stage_read_lux_distant_light_sample(const FreeusdStage* stage,
                                                              const char* light_path_utf8, double time,
                                                              FreeusdLuxDistantLightSample* out_sample);

/**
 * Count of bound blend-shape targets on a geom prim (``skel:blendShapes`` token count).
 * Missing geom or unbound prim reports @ref FREEUSD_ERR_NOT_FOUND.
 */
FREEUSD_C_API int freeusd_stage_read_geom_blend_shape_target_count(const FreeusdStage* stage,
                                                                 const char* geom_path_utf8, size_t* out_count);

/**
 * Blend-shape weight at @p target_index for @p geom_path_utf8 at @p time (local or animation-remapped).
 * @p out_weight must be non-NULL. Index out of range reports @ref FREEUSD_ERR_NOT_FOUND.
 */
FREEUSD_C_API int freeusd_stage_read_geom_blend_shape_weight(const FreeusdStage* stage, const char* geom_path_utf8,
                                                            size_t target_index, double time, float* out_weight);

/**
 * Build a joint skinning palette: ``out_palette[i] = joint_world[i] * inverse_bind[i]`` (row-major 4x4).
 * @p joint_count must be positive; each input/output matrix is 16 consecutive doubles.
 */
FREEUSD_C_API int freeusd_usdskel_compute_skinning_matrices(size_t joint_count, const double* joint_world_row_major,
                                                          const double* inverse_bind_row_major,
                                                          double* out_palette_row_major);

/**
 * CPU LBS using composed skeleton bind transforms and animation TRS at @p time.
 * @p in_points_xyz / @p out_points_xyz are @c 3 * @p point_count floats (xyz interleaved).
 * @p joint_indices / @p joint_weights length is @p point_count * @p influences_per_point.
 */
FREEUSD_C_API int freeusd_stage_deform_points_with_skeleton(const FreeusdStage* stage,
                                                            const char* skeleton_path_utf8,
                                                            const char* animation_path_utf8, double time,
                                                            size_t point_count, const float* in_points_xyz,
                                                            const int* joint_indices, const float* joint_weights,
                                                            size_t influences_per_point, float* out_points_xyz);

/** Recommended engine runtime mode (matches @c freeusd::usdUtils::EngineRuntimeMode). */
enum FreeusdEngineRuntimeMode {
  FREEUSD_ENGINE_RUNTIME_PREBAKED = 0,
  FREEUSD_ENGINE_RUNTIME_HYBRID = 1,
  FREEUSD_ENGINE_RUNTIME_EXPERIMENTAL_LIVE = 2,
};

/** Feature flags from @c freeusd::usdUtils::AssessEngineRuntimeSupport (0/1 fields). */
typedef struct FreeusdEngineRuntimeSupport {
  int recommended_mode;
  int uses_composed_layer_stack;
  int uses_references;
  int uses_payloads;
  int uses_inherits;
  int uses_specializes;
  int uses_variant_selection;
  int uses_variant_sets;
  int uses_relocates;
  int uses_prefix_substitutions;
  int uses_time_samples;
  int uses_relationships;
  int uses_custom_data;
  int uses_attribute_connections;
  int uses_skel_bound_meshes;
  int uses_blend_shapes;
  int uses_skel_animation;
  int uses_material_bindings;
  int uses_preview_surface;
  int uses_preview_surface_textures;
  int uses_lux_lights;
  int uses_composed_prim_kind;
  int uses_prim_active_opinions;
  int uses_kind_active_through_arcs;
  int uses_custom_data_through_arcs;
  int uses_physics_scenes;
  int uses_rigid_body_api;
  int uses_collision_api;
  int uses_physics_fixed_joints;
  int uses_open_vdb_assets;
  int uses_volumes;
} FreeusdEngineRuntimeSupport;

/**
 * Assess which engine runtime integration mode fits the composed stage.
 * @p out must be non-NULL.
 */
FREEUSD_C_API int freeusd_usdutils_assess_engine_runtime_support(const FreeusdStage* stage,
                                                                 FreeusdEngineRuntimeSupport* out);

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
 * On success returns @c 0-@c 3. On error returns the negation of a @ref FreeusdResult code (e.g. @c -1 invalid argument).
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

/**
 * Composed @c customData entry as integer (@c int32 / @c int64 / @c bool coerced to @c int64).
 * Missing keys and non-integer payloads report @ref FREEUSD_ERR_NOT_FOUND.
 */
FREEUSD_C_API int freeusd_stage_get_composed_prim_custom_data_int64(const FreeusdStage* stage,
                                                                    const char* prim_path_utf8,
                                                                    const char* key_utf8, int64_t* out_value);

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
