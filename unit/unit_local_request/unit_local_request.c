/*
 * Copyright (C) 2018 Jolla Ltd.
 * Contact: Slava Monich <slava.monich@jolla.com>
 *
 * You may use this file under the terms of BSD license as follows:
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *   1. Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *   2. Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *   3. Neither the name of Jolla Ltd nor the names of its contributors may
 *      be used to endorse or promote products derived from this software
 *      without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "test_common.h"

#include "gbinder_local_request_p.h"
#include "gbinder_output_data.h"
#include "gbinder_writer.h"
#include "gbinder_io.h"

#include <gutil_intarray.h>

static TestOpt test_opt;

#define BUFFER_OBJECT_SIZE_32 (24)
#define BUFFER_OBJECT_SIZE_64 (GBINDER_MAX_BUFFER_OBJECT_SIZE)
#define BINDER_OBJECT_SIZE_32 (16)
#define BINDER_OBJECT_SIZE_64 (GBINDER_MAX_BINDER_OBJECT_SIZE)

static
void
test_int_inc(
    void* data)
{
    (*((int*)data))++;
}

/*==========================================================================*
 * null
 *==========================================================================*/

static
void
test_null(
    void)
{
    GBinderWriter writer;
    int count = 0;

    g_assert(!gbinder_local_request_new(NULL, NULL));
    g_assert(!gbinder_local_request_ref(NULL));
    gbinder_local_request_unref(NULL);
    gbinder_local_request_init_writer(NULL, NULL);
    gbinder_local_request_init_writer(NULL, &writer);
    gbinder_local_request_cleanup(NULL, NULL, NULL);
    gbinder_local_request_cleanup(NULL, test_int_inc, &count);
    g_assert(count == 1);

    g_assert(!gbinder_local_request_data(NULL));
    g_assert(!gbinder_local_request_append_int32(NULL, 0));
    g_assert(!gbinder_local_request_append_int64(NULL, 0));
    g_assert(!gbinder_local_request_append_string8(NULL, NULL));
    g_assert(!gbinder_local_request_append_string16(NULL, NULL));
    g_assert(!gbinder_local_request_append_hidl_string(NULL, NULL));
    g_assert(!gbinder_local_request_append_hidl_string_vec(NULL, NULL, 0));
    g_assert(!gbinder_local_request_append_local_object(NULL, NULL));
    g_assert(!gbinder_local_request_append_remote_object(NULL, NULL));
}

/*==========================================================================*
 * cleanup
 *==========================================================================*/

static
void
test_cleanup(
    void)
{
    GBinderLocalRequest* req = gbinder_local_request_new(&gbinder_io_32, NULL);
    int count = 0;

    gbinder_local_request_cleanup(req, NULL, &count);
    gbinder_local_request_cleanup(req, test_int_inc, &count);
    gbinder_local_request_cleanup(req, test_int_inc, &count);
    g_assert(!count);

    gbinder_local_request_unref(req);
    g_assert(count == 2);
}

/*==========================================================================*
 * init_data
 *==========================================================================*/

static
void
test_init_data(
    void)
{
    const guint8 init_data[] = { 0x01, 0x02, 0x03, 0x04 };
    GBytes* init_bytes = g_bytes_new_static(init_data, sizeof(init_data));
    GBinderLocalRequest* req = gbinder_local_request_new
        (&gbinder_io_32, init_bytes);
    GBinderOutputData* data;

    data = gbinder_local_request_data(req);
    g_assert(!gbinder_output_data_offsets(data));
    g_assert(!gbinder_output_data_buffers_size(data));
    g_assert(data->bytes->len == sizeof(init_data));
    g_assert(!memcmp(data->bytes->data, init_data, data->bytes->len));
    g_assert(gbinder_local_request_ref(req) == req);
    gbinder_local_request_unref(req);
    gbinder_local_request_unref(req);

    req = gbinder_local_request_new(&gbinder_io_32, NULL);
    data = gbinder_local_request_data(req);
    g_assert(data->bytes);
    g_assert(!data->bytes->len);
    gbinder_local_request_unref(req);

    g_bytes_unref(init_bytes);
}

/*==========================================================================*
 * int32
 *==========================================================================*/

static
void
test_int32(
    void)
{
    const guint32 value = 1234567;
    GBinderLocalRequest* req = gbinder_local_request_new(&gbinder_io_32, NULL);
    GBinderOutputData* data;

    gbinder_local_request_append_int32(req, value);
    data = gbinder_local_request_data(req);
    g_assert(!gbinder_output_data_offsets(data));
    g_assert(!gbinder_output_data_buffers_size(data));
    g_assert(data->bytes->len == sizeof(value));
    g_assert(!memcmp(data->bytes->data, &value, data->bytes->len));
    gbinder_local_request_unref(req);
}

/*==========================================================================*
 * int64
 *==========================================================================*/

static
void
test_int64(
    void)
{
    const guint64 value = 123456789;
    GBinderLocalRequest* req = gbinder_local_request_new(&gbinder_io_32, NULL);
    GBinderOutputData* data;

    gbinder_local_request_append_int64(req, value);
    data = gbinder_local_request_data(req);
    g_assert(!gbinder_output_data_offsets(data));
    g_assert(!gbinder_output_data_buffers_size(data));
    g_assert(data->bytes->len == sizeof(value));
    g_assert(!memcmp(data->bytes->data, &value, data->bytes->len));
    gbinder_local_request_unref(req);
}

/*==========================================================================*
 * string8
 *==========================================================================*/

static
void
test_string8(
    void)
{
    /* The size of the string gets aligned at 4-byte boundary */
    static const char input[] = "test";
    static const guint8 output[] = { 't', 'e', 's', 't', 0, 0, 0, 0 };
    GBinderLocalRequest* req = gbinder_local_request_new(&gbinder_io_32, NULL);
    GBinderOutputData* data;

    gbinder_local_request_append_string8(req, input);
    data = gbinder_local_request_data(req);
    g_assert(!gbinder_output_data_offsets(data));
    g_assert(!gbinder_output_data_buffers_size(data));
    g_assert(data->bytes->len == sizeof(output));
    g_assert(!memcmp(data->bytes->data, output, data->bytes->len));
    gbinder_local_request_unref(req);

    /* NULL string doesn't get encoded at all (should it be?) */
    req = gbinder_local_request_new(&gbinder_io_32, NULL);
    gbinder_local_request_append_string8(req, NULL);
    data = gbinder_local_request_data(req);
    g_assert(!gbinder_output_data_offsets(data));
    g_assert(!gbinder_output_data_buffers_size(data));
    g_assert(!data->bytes->len);
    gbinder_local_request_unref(req);
}

/*==========================================================================*
 * string16
 *==========================================================================*/

static
void
test_string16(
    void)
{
    static const const char input[] = "x";
    static const guint8 output[] = {
        TEST_INT32_BYTES(1),
        TEST_INT16_BYTES('x'), 0x00, 0x00
    };
    const gint32 null_output = -1;
    GBinderLocalRequest* req = gbinder_local_request_new(&gbinder_io_32, NULL);
    GBinderOutputData* data;

    gbinder_local_request_append_string16(req, input);
    data = gbinder_local_request_data(req);
    g_assert(!gbinder_output_data_offsets(data));
    g_assert(!gbinder_output_data_buffers_size(data));
    g_assert(data->bytes->len == sizeof(output));
    g_assert(!memcmp(data->bytes->data, output, data->bytes->len));
    gbinder_local_request_unref(req);

    /* NULL string gets encoded as -1 */
    req = gbinder_local_request_new(&gbinder_io_32, NULL);
    gbinder_local_request_append_string16(req, NULL);
    data = gbinder_local_request_data(req);
    g_assert(!gbinder_output_data_offsets(data));
    g_assert(!gbinder_output_data_buffers_size(data));
    g_assert(data->bytes->len == sizeof(null_output));
    g_assert(!memcmp(data->bytes->data, &null_output, data->bytes->len));
    gbinder_local_request_unref(req);
}

/*==========================================================================*
 * hidl_string
 *==========================================================================*/

static
void
test_hidl_string(
    void)
{
    GBinderLocalRequest* req = gbinder_local_request_new(&gbinder_io_32, NULL);
    GBinderOutputData* data;
    GUtilIntArray* offsets;

    gbinder_local_request_append_hidl_string(req, NULL);
    data = gbinder_local_request_data(req);
    offsets = gbinder_output_data_offsets(data);
    g_assert(offsets->count == 1);
    g_assert(offsets->data[0] == 0);
    g_assert(gbinder_output_data_buffers_size(data) == sizeof(HidlString));
    g_assert(data->bytes->len == BUFFER_OBJECT_SIZE_32);
    gbinder_local_request_unref(req);
}

/*==========================================================================*
 * hidl_string_vec
 *==========================================================================*/

static
void
test_hidl_string_vec(
    void)
{
    GBinderLocalRequest* req = gbinder_local_request_new(&gbinder_io_32, NULL);
    GBinderOutputData* data;
    GUtilIntArray* offsets;

    gbinder_local_request_append_hidl_string_vec(req, NULL, 0);
    data = gbinder_local_request_data(req);
    offsets = gbinder_output_data_offsets(data);
    g_assert(offsets->count == 1);
    g_assert(offsets->data[0] == 0);
    g_assert(gbinder_output_data_buffers_size(data) == sizeof(HidlVec));
    g_assert(data->bytes->len == BUFFER_OBJECT_SIZE_32);
    gbinder_local_request_unref(req);
}

/*==========================================================================*
 * local_object
 *==========================================================================*/

static
void
test_local_object(
    void)
{
    GBinderLocalRequest* req = gbinder_local_request_new(&gbinder_io_32, NULL);
    GBinderOutputData* data;
    GUtilIntArray* offsets;

    gbinder_local_request_append_local_object(req, NULL);
    data = gbinder_local_request_data(req);
    offsets = gbinder_output_data_offsets(data);
    g_assert(offsets);
    g_assert(offsets->count == 1);
    g_assert(offsets->data[0] == 0);
    g_assert(!gbinder_output_data_buffers_size(data));
    g_assert(data->bytes->len == BINDER_OBJECT_SIZE_32);
    gbinder_local_request_unref(req);
}

/*==========================================================================*
 * remote_object
 *==========================================================================*/

static
void
test_remote_object(
    void)
{
    GBinderLocalRequest* req = gbinder_local_request_new(&gbinder_io_32, NULL);
    GBinderOutputData* data;
    GUtilIntArray* offsets;

    gbinder_local_request_append_remote_object(req, NULL);
    data = gbinder_local_request_data(req);
    offsets = gbinder_output_data_offsets(data);
    g_assert(offsets);
    g_assert(offsets->count == 1);
    g_assert(offsets->data[0] == 0);
    g_assert(!gbinder_output_data_buffers_size(data));
    g_assert(data->bytes->len == BINDER_OBJECT_SIZE_32);
    gbinder_local_request_unref(req);
}

/*==========================================================================*
 * Common
 *==========================================================================*/

#define TEST_PREFIX "/local_request/"

int main(int argc, char* argv[])
{
    g_test_init(&argc, &argv, NULL);
    g_test_add_func(TEST_PREFIX "null", test_null);
    g_test_add_func(TEST_PREFIX "cleanup", test_cleanup);
    g_test_add_func(TEST_PREFIX "init_data", test_init_data);
    g_test_add_func(TEST_PREFIX "int32", test_int32);
    g_test_add_func(TEST_PREFIX "int64", test_int64);
    g_test_add_func(TEST_PREFIX "string8", test_string8);
    g_test_add_func(TEST_PREFIX "string16", test_string16);
    g_test_add_func(TEST_PREFIX "hidl_string", test_hidl_string);
    g_test_add_func(TEST_PREFIX "hidl_string_vec", test_hidl_string_vec);
    g_test_add_func(TEST_PREFIX "local_object", test_local_object);
    g_test_add_func(TEST_PREFIX "remote_object", test_remote_object);
    test_init(&test_opt, argc, argv);
    return g_test_run();
}

/*
 * Local Variables:
 * mode: C
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 */