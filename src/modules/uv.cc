/**
 * @module ssc.uv
 * @description Exports libuv symbols and makes them available to the module importer
 * @example
 * import ssc.uv;
 * namespace ssc {
 *   uv_loop_t loop;
 *   uv_loop_init(&loop);
 * }
 */
export module ssc.uv;
export import libuv;
