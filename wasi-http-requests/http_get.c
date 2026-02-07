/**
 * http_get.c - Make an outbound HTTP GET request using wasi:http
 *
 * This demonstrates making HTTP requests from C compiled to WASI,
 * using the wasi:http/outgoing-handler component model interface.
 *
 * Build:
 *   $WASI_SDK/bin/clang --target=wasm32-wasip2 \
 *     --sysroot=$WASI_SDK/share/wasi-sysroot \
 *     -I. -o http_get.wasm http_get.c generated/imports.c generated/imports_component_type.o
 *
 * Run:
 *   wasmtime run -S http -S inherit-network http_get.wasm
 */

#include "generated/imports.h"
#include <stdio.h>
#include <string.h>

// Helper to create an imports_string_t from a C string literal (no copy)
#define STR(s) ((imports_string_t){ .ptr = (uint8_t*)(s), .len = strlen(s) })

// wit-bindgen C convention: bool-returning functions return true=success, false=error

int main(void) {
    // 1. Create empty headers
    wasi_http_types_own_fields_t headers = wasi_http_types_constructor_fields();

    // 2. Create the outgoing request with those headers
    wasi_http_types_own_outgoing_request_t request =
        wasi_http_types_constructor_outgoing_request(headers);

    // 3. Get a borrow to configure the request
    wasi_http_types_borrow_outgoing_request_t req_borrow =
        wasi_http_types_borrow_outgoing_request(request);

    // 4. Set method to GET
    wasi_http_types_method_t method = { .tag = WASI_HTTP_TYPES_METHOD_GET };
    wasi_http_types_method_outgoing_request_set_method(req_borrow, &method);

    // 5. Set scheme to HTTPS
    wasi_http_types_scheme_t scheme = { .tag = WASI_HTTP_TYPES_SCHEME_HTTPS };
    wasi_http_types_method_outgoing_request_set_scheme(req_borrow, &scheme);

    // 6. Set authority (host)
    imports_string_t authority = STR("httpbin.org");
    wasi_http_types_method_outgoing_request_set_authority(req_borrow, &authority);

    // 7. Set path with query string
    imports_string_t path = STR("/get?source=wasi-c");
    wasi_http_types_method_outgoing_request_set_path_with_query(req_borrow, &path);

    // 8. Send the request via outgoing-handler
    wasi_http_outgoing_handler_own_future_incoming_response_t future;
    wasi_http_outgoing_handler_error_code_t send_err;
    if (!wasi_http_outgoing_handler_handle(request, NULL, &future, &send_err)) {
        fprintf(stderr, "Error: failed to send request (error tag: %d)\n", send_err.tag);
        if (send_err.tag == WASI_HTTP_TYPES_ERROR_CODE_INTERNAL_ERROR &&
            send_err.val.internal_error.is_some)
            fprintf(stderr, "  Detail: %.*s\n",
                    (int)send_err.val.internal_error.val.len,
                    send_err.val.internal_error.val.ptr);
        return 1;
    }
    printf("Request sent, waiting for response...\n");

    // 9. Block on the future until the response arrives
    wasi_http_types_borrow_future_incoming_response_t future_borrow =
        wasi_http_types_borrow_future_incoming_response(future);
    wasi_http_types_own_pollable_t pollable =
        wasi_http_types_method_future_incoming_response_subscribe(future_borrow);
    wasi_io_poll_method_pollable_block(
        wasi_io_poll_borrow_pollable(pollable));
    wasi_io_poll_pollable_drop_own(pollable);

    // 10. Get the response from the future
    //     get() returns true when option is Some (has value)
    //     ret is result<result<incoming-response, error-code>, void>
    wasi_http_types_result_result_own_incoming_response_error_code_void_t future_result;
    if (!wasi_http_types_method_future_incoming_response_get(future_borrow, &future_result)) {
        fprintf(stderr, "Error: future has no value yet\n");
        return 1;
    }
    if (future_result.is_err) {
        fprintf(stderr, "Error: request failed (outer result error)\n");
        return 1;
    }
    if (future_result.val.ok.is_err) {
        fprintf(stderr, "Error: HTTP error (error tag: %d)\n",
                future_result.val.ok.val.err.tag);
        return 1;
    }

    wasi_http_types_own_incoming_response_t response = future_result.val.ok.val.ok;
    wasi_http_types_future_incoming_response_drop_own(future);

    // 11. Read status code
    wasi_http_types_borrow_incoming_response_t resp_borrow =
        wasi_http_types_borrow_incoming_response(response);
    wasi_http_types_status_code_t status =
        wasi_http_types_method_incoming_response_status(resp_borrow);
    printf("Status: %d\n", status);

    // 12. Read response headers
    wasi_http_types_own_headers_t resp_headers =
        wasi_http_types_method_incoming_response_headers(resp_borrow);
    imports_list_tuple2_field_name_field_value_t header_entries;
    wasi_http_types_method_fields_entries(
        wasi_http_types_borrow_fields(resp_headers), &header_entries);
    printf("Headers:\n");
    for (size_t i = 0; i < header_entries.len; i++) {
        imports_string_t *name = &header_entries.ptr[i].f0;
        wasi_http_types_field_value_t *value = &header_entries.ptr[i].f1;
        printf("  %.*s: %.*s\n",
               (int)name->len, name->ptr,
               (int)value->len, value->ptr);
    }
    wasi_http_types_fields_drop_own(resp_headers);

    // 13. Read response body
    wasi_http_types_own_incoming_body_t body;
    if (!wasi_http_types_method_incoming_response_consume(resp_borrow, &body)) {
        fprintf(stderr, "Error: failed to consume response body\n");
        return 1;
    }

    wasi_http_types_own_input_stream_t body_stream;
    if (!wasi_http_types_method_incoming_body_stream(
            wasi_http_types_borrow_incoming_body(body), &body_stream)) {
        fprintf(stderr, "Error: failed to get body stream\n");
        return 1;
    }

    printf("\nBody:\n");
    wasi_io_streams_borrow_input_stream_t stream_borrow =
        wasi_io_streams_borrow_input_stream(body_stream);

    while (1) {
        imports_list_u8_t chunk;
        wasi_io_streams_stream_error_t read_err;
        if (!wasi_io_streams_method_input_stream_blocking_read(
                stream_borrow, 65536, &chunk, &read_err)) {
            // Check if it's end-of-stream (closed)
            if (read_err.tag == WASI_IO_STREAMS_STREAM_ERROR_CLOSED) {
                break;
            }
            fprintf(stderr, "Error: read failed (tag: %d)\n", read_err.tag);
            break;
        }
        if (chunk.len > 0) {
            fwrite(chunk.ptr, 1, chunk.len, stdout);
        }
        imports_list_u8_free(&chunk);
    }
    printf("\n");

    // 14. Clean up
    wasi_io_streams_input_stream_drop_own(body_stream);
    wasi_http_types_static_incoming_body_finish(body);
    wasi_http_types_incoming_response_drop_own(response);

    return 0;
}
