// Copyright (c) Open Enclave SDK contributors.
// Licensed under the MIT License.

#include <openenclave/attestation/attester.h>
#include <openenclave/attestation/verifier.h>
#include <openenclave/bits/sgx/sgxtypes.h>
#include <openenclave/enclave.h>
#include <openenclave/internal/report.h>
#include <openenclave/internal/sgx/plugin.h>
#include <openenclave/internal/tests.h>
#include <openenclave/internal/trace.h>
#include <stdio.h>
#include <string.h>

#include "../../../common/sgx/quote.h"
#include "../plugin/tests.h"
#include "plugin_t.h"

static oe_uuid_t sgx_ecdsa_uuid = {OE_FORMAT_UUID_SGX_ECDSA_P256};
static oe_uuid_t sgx_local_uuid = {OE_FORMAT_UUID_SGX_LOCAL_ATTESTATION};

void run_runtime_test()
{
    test_runtime();
}

void register_sgx()
{
    printf("====== running register_sgx\n");

    OE_TEST_CODE(oe_attester_initialize(), OE_OK);
    register_verifier();
}

void unregister_sgx()
{
    printf("====== running unregister_sgx\n");

    OE_TEST_CODE(oe_attester_shutdown(), OE_OK);
    unregister_verifier();
}

static void _test_sgx_remote()
{
    printf("====== running _test_sgx_remote\n");
    uint8_t* evidence = NULL;
    size_t evidence_size = 0;
    uint8_t* endorsements = NULL;
    size_t endorsements_size = 0;
    oe_uuid_t selected_format;

    OE_TEST_CODE(
        oe_attester_select_format(&sgx_ecdsa_uuid, 1, &selected_format), OE_OK);

    // Get a remote attestation report.
    printf("====== running _test_sgx_remote #1: Just evidence\n");
    OE_TEST_CODE(
        oe_get_evidence(
            &selected_format,
            NULL,
            0,
            NULL,
            0,
            &evidence,
            &evidence_size,
            NULL,
            0),
        OE_OK);

    printf("    ====== evidence_size=%d\n", evidence_size);

    verify_sgx_evidence(evidence, evidence_size, NULL, 0, NULL, 0, false);

    OE_TEST(oe_free_evidence(evidence) == OE_OK);

    // Get a remote report with endorsements.
    printf("====== running _test_sgx_remote #2: + Endorsements\n");
    OE_TEST_CODE(
        oe_get_evidence(
            &selected_format,
            NULL,
            0,
            NULL,
            0,
            &evidence,
            &evidence_size,
            &endorsements,
            &endorsements_size),
        OE_OK);

    printf(
        "    ====== evidence_size=%d endorsements_size=%d\n",
        evidence_size,
        endorsements_size);

    verify_sgx_evidence(
        evidence,
        evidence_size,
        endorsements,
        endorsements_size,
        NULL,
        0,
        false);

    OE_TEST(oe_free_evidence(evidence) == OE_OK);
    OE_TEST(oe_free_endorsements(endorsements) == OE_OK);

    // Get a remote report with both.
    printf("====== running _test_sgx_remote #3: + Claims\n");
    OE_TEST_CODE(
        oe_get_evidence(
            &selected_format,
            test_claims,
            NUM_TEST_CLAIMS,
            NULL,
            0,
            &evidence,
            &evidence_size,
            &endorsements,
            &endorsements_size),
        OE_OK);

    printf(
        "    ====== evidence_size=%d endorsements_size=%d claims_length=%d\n",
        evidence_size,
        endorsements_size,
        NUM_TEST_CLAIMS);

    verify_sgx_evidence(
        evidence,
        evidence_size,
        endorsements,
        endorsements_size,
        test_claims,
        NUM_TEST_CLAIMS,
        false);

    printf(
        "====== running _test_sgx_remote #3: + Claims, using host_verify()\n");
    OE_TEST(
        host_verify(evidence, evidence_size, endorsements, endorsements_size) ==
        OE_OK);

    OE_TEST(oe_free_evidence(evidence) == OE_OK);
    OE_TEST(oe_free_endorsements(endorsements) == OE_OK);
}

static void _test_sgx_local()
{
    uint8_t* target = NULL;
    size_t target_size = 0;
    uint8_t* evidence = NULL;
    size_t evidence_size = 0;
    oe_uuid_t selected_format;

    printf("====== running _test_sgx_local\n");

    OE_TEST_CODE(
        oe_attester_select_format(&sgx_local_uuid, 1, &selected_format), OE_OK);

    printf("====== running _test_sgx_local #0: Getting target info.\n");
    OE_TEST(oe_verifier_get_format_settings(
        &selected_format, &target, &target_size));

    // Only evidence.
    printf("====== running _test_sgx_local #1: Just evidence\n");
    OE_TEST(
        oe_get_evidence(
            &selected_format,
            NULL,
            0,
            target,
            target_size,
            &evidence,
            &evidence_size,
            NULL,
            0) == OE_OK);

    verify_sgx_evidence(evidence, evidence_size, NULL, 0, NULL, 0, true);

    OE_TEST(oe_free_evidence(evidence) == OE_OK);

    // Evidence + claims.
    printf("====== running _test_sgx_local #2: + Claims\n");
    OE_TEST(
        oe_get_evidence(
            &selected_format,
            test_claims,
            NUM_TEST_CLAIMS,
            target,
            target_size,
            &evidence,
            &evidence_size,
            NULL,
            0) == OE_OK);

    verify_sgx_evidence(
        evidence, evidence_size, NULL, 0, test_claims, NUM_TEST_CLAIMS, true);

    OE_TEST(oe_free_evidence(evidence) == OE_OK);
    oe_verifier_free_format_settings(target);
}

void test_sgx()
{
    printf("====== running test_sgx\n");

    _test_sgx_remote();
    _test_sgx_local();
}

OE_SET_ENCLAVE_SGX(
    1,    /* ProductID */
    1,    /* SecurityVersion */
    true, /* Debug */
    128,  /* NumHeapPages */
    128,  /* NumStackPages */
    1);   /* NumTCS */
