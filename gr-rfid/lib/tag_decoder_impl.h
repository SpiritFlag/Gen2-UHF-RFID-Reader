/* -*- c++ -*- */
/*
 * Copyright 2015 <Nikos Kargas (nkargas@isc.tuc.gr)>.
 *
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#ifndef INCLUDED_RFID_TAG_DECODER_IMPL_H
#define INCLUDED_RFID_TAG_DECODER_IMPL_H

#include <rfid/tag_decoder.h>
#include <vector>
#include "rfid/global_vars.h"
#include <time.h>
#include <numeric>
#include <fstream>

#define DEBUG_DECODER_RN16
#define DEBUG_DECODER_EPC

namespace gr
{
  namespace rfid
  {
    class tag_decoder_impl : public tag_decoder
    {
      private:

        float n_samples_TAG_BIT;
        int s_rate;
        std::vector<float> pulse_bit;
        float T_global;
        gr_complex h_est;
        char * char_bits;
        FILE *preamble_fp;
        int success_count;

        int tag_sync(float* in, int size);
        int determine_first_mask_level(float* in, int index);
        int decode_single_bit(float* in, int index, int mask_level, float* ret_corr);
        std::vector<float> tag_detection(float* in, int index, int n_expected_bit);
        int check_crc(char * bits, int num_bits);

        // debug_message
        std::string current_round_slot;
        std::ofstream debug_log;
        #ifdef DEBUG_DECODER_RN16
        std::ofstream debug_decoder_RN16_i;
        std::ofstream debug_decoder_RN16_q;
        std::ofstream debug_decoder_RN16;
        #endif
        #ifdef DEBUG_DECODER_EPC
        std::ofstream debug_decoder_EPC_i;
        std::ofstream debug_decoder_EPC_q;
        std::ofstream debug_decoder_EPC;
        #endif

      public:
        tag_decoder_impl(int sample_rate, std::vector<int> output_sizes);
        ~tag_decoder_impl();

        void forecast (int noutput_items, gr_vector_int &ninput_items_required);

        int general_work(int noutput_items,
            gr_vector_int &ninput_items,
            gr_vector_const_void_star &input_items,
            gr_vector_void_star &output_items);
    };

  } // namespace rfid
} // namespace gr

#endif /* INCLUDED_RFID_TAG_DECODER_IMPL_H */
