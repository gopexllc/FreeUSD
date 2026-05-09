/**
 * @file freeusd.h
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

FREEUSD_C_API void freeusd_stage_free(FreeusdStage* stage);

/**
 * List direct child prim paths under @p parent_prim_utf8 (union across composed layers; stable sort).
 * On @ref FREEUSD_OK, sets @p *out_paths to a malloc'd array of @p *out_count malloc'd UTF-8 strings.
 * Free with @ref freeusd_path_list_free.
 */
FREEUSD_C_API int freeusd_stage_list_child_paths(const FreeusdStage* stage, const char* parent_prim_utf8,
                                                  char*** out_paths, size_t* out_count);

FREEUSD_C_API void freeusd_path_list_free(char** paths, size_t count);

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

/** 1 if prim path exists on the composed stage, 0 if not, negative @ref FreeusdResult on error. */
FREEUSD_C_API int freeusd_stage_prim_is_valid(const FreeusdStage* stage, const char* prim_path_utf8);

/**
 * Read evaluated attribute as double at @p time (strongest opinion; follows
 * attribute connections where implemented).
 * @return @ref FREEUSD_OK or error code; @p out_value unchanged on failure.
 */
FREEUSD_C_API int freeusd_stage_read_field_double(const FreeusdStage* stage, const char* prim_path_utf8,
                                                  const char* attr_name_utf8, double time, double* out_value);

/** 1 if any composed layer authors @p attr_name_utf8 on @p prim_path_utf8, 0 if not, negative @ref FreeusdResult on error. */
FREEUSD_C_API int freeusd_stage_has_field_opinion(const FreeusdStage* stage, const char* prim_path_utf8,
                                                  const char* attr_name_utf8);

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
 * Composed prim active flag (strongest opinion; default true if no opinion).
 * @p out_active receives 0 or 1.
 */
FREEUSD_C_API int freeusd_stage_resolve_prim_active(const FreeusdStage* stage, const char* prim_path_utf8,
                                                    int* out_active);

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

#ifdef __cplusplus
}
#endif

#endif /* FREEUSD_C_FREEUSD_H */
