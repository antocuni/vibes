/**
 * http_post.c - Make an outbound HTTP POST request using wasi:http
 *
 * This demonstrates sending a POST request with a JSON body from C
 * compiled to WASI, using the wasi:http/outgoing-handler interface.
 *
 * Build:
 *   $WASI_SDK/bin/clang --target=wasm32-wasip2 \
 *     --sysroot=$WASI_SDK/share/wasi-sysroot \
 *     -I. -o http_post.wasm http_post.c generated/imports.c generated/imports_component_type.o
 *
 * Run:
 *   wasmtime run -S http -S inherit-network http_post.wasm
 */

#include "generated/imports.h"
#include <stdio.h>
#include <string.h>

#define STR(s) ((imports_string_t){ .ptr = (uint8_t*)(s), .len = strlen(s) })

// wit-bindgen C convention: bool-returning functions return true=success, false=error

int main(void) {
    // 1. Create headers with Content-Type
    wasi_http_types_own_fields_t headers = wasi_http_types_constructor_fields();
    {
        wasi_http_types_borrow_fields_t hdr_borrow = wasi_http_types_borrow_fields(headers);
        wasi_http_types_field_name_t ct_name = STR("content-type");
        wasi_http_types_field_value_t ct_value = {
            .ptr = (uint8_t*)"application/json",
            .len = strlen("application/json")
        };
        wasi_http_types_header_error_t hdr_err;
        wasi_http_types_method_fields_append(hdr_borrow, &ct_name, &ct_value, &hdr_err);
    }

    // 2. Create the outgoing request
    wasi_http_types_own_outgoing_request_t request =
        wasi_http_types_constructor_outgoing_request(headers);

    wasi_http_types_borrow_outgoing_request_t req_borrow =
        wasi_http_types_borrow_outgoing_request(request);

    // 3. Configure: POST https://httpbin.org/post
    wasi_http_types_method_t method = { .tag = WASI_HTTP_TYPES_METHOD_POST };
    wasi_http_types_method_outgoing_request_set_method(req_borrow, &method);

    wasi_http_types_scheme_t scheme = { .tag = WASI_HTTP_TYPES_SCHEME_HTTPS };
    wasi_http_types_method_outgoing_request_set_scheme(req_borrow, &scheme);

    imports_string_t authority = STR("httpbin.org");
    wasi_http_types_method_outgoing_request_set_authority(req_borrow, &authority);

    imports_string_t path = STR("/post");
    wasi_http_types_method_outgoing_request_set_path_with_query(req_borrow, &path);

    // 4. Write the request body
    wasi_http_types_own_outgoing_body_t req_body;
    if (!wasi_http_types_method_outgoing_request_body(req_borrow, &req_body)) {
        fprintf(stderr, "Error: failed to get request body\n");
        return 1;
    }

    wasi_http_types_own_output_stream_t req_body_stream;
    if (!wasi_http_types_method_outgoing_body_write(
            wasi_http_types_borrow_outgoing_body(req_body), &req_body_stream)) {
        fprintf(stderr, "Error: failed to get body output stream\n");
        return 1;
    }

    const char *json_body = "{\"message\": \"Hello from WASI!\", \"source\": \"C via wasi:http\"}";
    imports_list_u8_t body_data = {
        .ptr = (uint8_t*)json_body,
        .len = strlen(json_body)
    };
    wasi_io_streams_stream_error_t write_err;
    if (!wasi_io_streams_method_output_stream_blocking_write_and_flush(
            wasi_io_streams_borrow_output_stream(req_body_stream),
            &body_data, &write_err)) {
        fprintf(stderr, "Error: failed to write request body\n");
        return 1;
    }

    // Close the output stream and finish the body
    wasi_io_streams_output_stream_drop_own(req_body_stream);
    wasi_http_types_error_code_t finish_err;
    if (!wasi_http_types_static_outgoing_body_finish(req_body, NULL, &finish_err)) {
        fprintf(stderr, "Error: failed to finish outgoing body\n");
        return 1;
    }

    // 5. Send the request
    wasi_http_outgoing_handler_own_future_incoming_response_t future;
    wasi_http_outgoing_handler_error_code_t send_err;
    if (!wasi_http_outgoing_handler_handle(request, NULL, &future, &send_err)) {
        fprintf(stderr, "Error: failed to send request (error tag: %d)\n", send_err.tag);
        return 1;
    }
    printf("POST request sent, waiting for response...\n");

    // 6. Block on the future
    wasi_http_types_borrow_future_incoming_response_t future_borrow =
        wasi_http_types_borrow_future_incoming_response(future);
    wasi_http_types_own_pollable_t pollable =
        wasi_http_types_method_future_incoming_response_subscribe(future_borrow);
    wasi_io_poll_method_pollable_block(
        wasi_io_poll_borrow_pollable(pollable));
    wasi_io_poll_pollable_drop_own(pollable);

    // 7. Get the response
    wasi_http_types_result_result_own_incoming_response_error_code_void_t future_result;
    if (!wasi_http_types_method_future_incoming_response_get(future_borrow, &future_result)) {
        fprintf(stderr, "Error: future has no value yet\n");
        return 1;
    }
    if (future_result.is_err) {
        fprintf(stderr, "Error: request failed\n");
        return 1;
    }
    if (future_result.val.ok.is_err) {
        fprintf(stderr, "Error: HTTP error (tag: %d)\n",
                future_result.val.ok.val.err.tag);
        return 1;
    }

    wasi_http_types_own_incoming_response_t response = future_result.val.ok.val.ok;
    wasi_http_types_future_incoming_response_drop_own(future);

    // 8. Print status and headers
    wasi_http_types_borrow_incoming_response_t resp_borrow =
        wasi_http_types_borrow_incoming_response(response);
    printf("Status: %d\n",
           wasi_http_types_method_incoming_response_status(resp_borrow));

    wasi_http_types_own_headers_t resp_headers =
        wasi_http_types_method_incoming_response_headers(resp_borrow);
    imports_list_tuple2_field_name_field_value_t header_entries;
    wasi_http_types_method_fields_entries(
        wasi_http_types_borrow_fields(resp_headers), &header_entries);
    printf("Headers:\n");
    for (size_t i = 0; i < header_entries.len; i++) {
        printf("  %.*s: %.*s\n",
               (int)header_entries.ptr[i].f0.len, header_entries.ptr[i].f0.ptr,
               (int)header_entries.ptr[i].f1.len, header_entries.ptr[i].f1.ptr);
    }
    wasi_http_types_fields_drop_own(resp_headers);

    // 9. Read response body
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
            if (read_err.tag == WASI_IO_STREAMS_STREAM_ERROR_CLOSED)
                break;
            fprintf(stderr, "Error: read failed\n");
            break;
        }
        if (chunk.len > 0)
            fwrite(chunk.ptr, 1, chunk.len, stdout);
        imports_list_u8_free(&chunk);
    }
    printf("\n");

    // 10. Clean up
    wasi_io_streams_input_stream_drop_own(body_stream);
    wasi_http_types_static_incoming_body_finish(body);
    wasi_http_types_incoming_response_drop_own(response);

    return 0;
}
