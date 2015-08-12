/*
 * Copyright (C) 2014 BlueKitchen GmbH
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holders nor the names of
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 * 4. Any redistribution, use, or modification is done solely for
 *    personal benefit and not for any commercial purpose or for
 *    monetary gain.
 *
 * THIS SOFTWARE IS PROVIDED BY BLUEKITCHEN GMBH AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL MATTHIAS
 * RINGWALD OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
 * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * Please inquire about commercial licensing options at 
 * contact@bluekitchen-gmbh.com
 *
 */


#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "CppUTest/TestHarness.h"
#include "CppUTest/CommandLineTestRunner.h"

#include "hfp.h"

void hfp_parse(hfp_connection_t * context, uint8_t byte);
hfp_generic_status_indicator_t * get_hfp_generic_status_indicators();
void set_hfp_generic_status_indicators(hfp_generic_status_indicator_t * indicators, int indicator_nr);

hfp_ag_indicator_t * get_hfp_ag_indicators();
int get_hfp_ag_indicators_nr();
void set_hfp_ag_indicators(hfp_ag_indicator_t * indicators, int indicator_nr);

static int hf_indicators_nr = 3;
static hfp_generic_status_indicator_t hf_indicators[] = {
    {1, 1},
    {2, 1},
    {3, 1}
};

static int hfp_ag_indicators_nr = 7;
static hfp_ag_indicator_t hfp_ag_indicators[] = {
    {1, "service",   0, 1, 1, 0, 0},
    {2, "call",      0, 1, 0, 1, 1},
    {3, "callsetup", 0, 3, 0, 1, 1},
    {4, "battchg",   0, 5, 3, 0, 0},
    {5, "signal",    0, 5, 5, 0, 0},
    {6, "roam",      0, 1, 0, 0, 0},
    {7, "callheld",  0, 2, 0, 1, 1}
};


static  hfp_connection_t context;

TEST_GROUP(HFPParser){
    char packet[200];
    int pos;
    int offset;

    void setup(void){
        context.parser_state = HFP_PARSER_CMD_HEADER;
        context.parser_item_index = 0;
        context.line_size = 0;
        memset(packet,0, sizeof(packet));
    }
};

TEST(HFPParser, HFP_AG_SUPPORTED_FEATURES){
    sprintf(packet, "\r\nAT%s=0000001111101111\r\n", HFP_SUPPORTED_FEATURES);
    for (pos = 0; pos < strlen(packet); pos++){
        hfp_parse(&context, packet[pos]);
    }
    CHECK_EQUAL(HFP_CMD_SUPPORTED_FEATURES, context.command);
    CHECK_EQUAL(1007, context.remote_supported_features);
}


TEST(HFPParser, HFP_AG_AVAILABLE_CODECS){
    sprintf(packet, "\r\nAT%s=0,1,2\r\n", HFP_AVAILABLE_CODECS);
    for (pos = 0; pos < strlen(packet); pos++){
        hfp_parse(&context, packet[pos]);
    }
    CHECK_EQUAL(HFP_CMD_AVAILABLE_CODECS, context.command);
    CHECK_EQUAL(3, context.remote_codecs_nr);
    for (pos = 0; pos < 3; pos++){
        CHECK_EQUAL(pos, context.remote_codecs[pos]);
    }   
}

TEST(HFPParser, HFP_AG_GENERIC_STATUS_INDICATOR){
    sprintf(packet, "\r\nAT%s=0,1\r\n", HFP_GENERIC_STATUS_INDICATOR);
    context.sent_command = HFP_CMD_GENERIC_STATUS_INDICATOR;

    for (pos = 0; pos < strlen(packet); pos++){
        hfp_parse(&context, packet[pos]);
    }
    
    CHECK_EQUAL(HFP_CMD_GENERIC_STATUS_INDICATOR, context.command);
    CHECK_EQUAL(2, context.generic_status_indicators_nr);
    
    for (pos = 0; pos < context.generic_status_indicators_nr; pos++){
        CHECK_EQUAL(pos, context.generic_status_indicators[pos].uuid);
    } 
}

TEST(HFPParser, HFP_AG_ENABLE_INDICATOR_STATUS_UPDATE){
    sprintf(packet, "\r\nAT%s=3,0,0,1\r\n", HFP_ENABLE_STATUS_UPDATE_FOR_AG_INDICATORS);
    for (pos = 0; pos < strlen(packet); pos++){
        hfp_parse(&context, packet[pos]);
    }
    CHECK_EQUAL(HFP_CMD_ENABLE_INDICATOR_STATUS_UPDATE, context.command);
    CHECK_EQUAL(1, context.enable_status_update_for_ag_indicators);
}



TEST(HFPParser, HFP_AG_ENABLE_INDIVIDUAL_INDICATOR_STATUS_UPDATE){
    set_hfp_ag_indicators((hfp_ag_indicator_t *)&hfp_ag_indicators, hfp_ag_indicators_nr);
    context.ag_indicators_nr = hfp_ag_indicators_nr;
    memcpy(context.ag_indicators, hfp_ag_indicators, hfp_ag_indicators_nr * sizeof(hfp_ag_indicator_t));
    
    for (pos = 0; pos < hfp_ag_indicators_nr; pos++){
        CHECK_EQUAL(get_hfp_ag_indicators()[pos].index, hfp_ag_indicators[pos].index);
        CHECK_EQUAL(get_hfp_ag_indicators()[pos].enabled, hfp_ag_indicators[pos].enabled);
        CHECK_EQUAL(context.ag_indicators[pos].index, hfp_ag_indicators[pos].index);
        CHECK_EQUAL(context.ag_indicators[pos].enabled, hfp_ag_indicators[pos].enabled);
    }
    sprintf(packet, "\r\nAT%s=0,0,0,0,0,0,0\r\n", 
        HFP_UPDATE_ENABLE_STATUS_FOR_INDIVIDUAL_AG_INDICATORS);
    for (pos = 0; pos < strlen(packet); pos++){
        hfp_parse(&context, packet[pos]);
    }

    CHECK_EQUAL(HFP_CMD_ENABLE_INDIVIDUAL_AG_INDICATOR_STATUS_UPDATE, context.command);

    for (pos = 0; pos < hfp_ag_indicators_nr; pos++){
        if (get_hfp_ag_indicators()[pos].mandatory){
            CHECK_EQUAL(get_hfp_ag_indicators()[pos].enabled, 1);
            CHECK_EQUAL(context.ag_indicators[pos].enabled, 1);
        } else {
            CHECK_EQUAL(get_hfp_ag_indicators()[pos].enabled, 0);
            CHECK_EQUAL(context.ag_indicators[pos].enabled, 0);
        }
    }

}

int main (int argc, const char * argv[]){
    return CommandLineTestRunner::RunAllTests(argc, argv);
}