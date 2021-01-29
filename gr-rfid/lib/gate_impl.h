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

#ifndef INCLUDED_RFID_GATE_IMPL_H
#define INCLUDED_RFID_GATE_IMPL_H

#include <rfid/gate.h>
#include <vector>
#include "rfid/global_vars.h"
#include <fstream>


namespace gr {
  namespace rfid {
    class gate_impl : public gate
    {
      private:
        GATE_STATUS     prev_gate_status = GATE_CLOSED;

        enum SIGNAL_STATE {NEG_EDGE, POS_EDGE};

        int n_samples, n_samples_T1, n_samples_TAG_BIT, n_samples_PW, n_samples_RTCAL, n_samples_TRCAL, n_samples_DELIM;
        int sample_rate;

        gr_complex avg_iq;
        gr_complex avg_dc;

        unsigned int iq_count = 0;
        int max_count = 0;
        int num_pulses;

        float amp_pos_threshold = 0;
        float amp_neg_threshold = 0;

        std::vector<uint8_t> reader_signal_decode(const gr_complex * in_data, int * read_idx, int expected_bit_num);
        uint8_t decode_onebit(const gr_complex * in_data, int * read_idx);

        SIGNAL_STATE signal_state;

        std::ofstream log;
        std::vector<gr_complex> gate_log_samples;

        void gateLogSave(void);

        class reader_decoder
        {
          private:
            enum Decode_State {DELIMITER, DATA0, RTCAL, TRCAL, DATAS} decode_state;
            bool is_preamble = true;
            bool up_down_state;
            std::vector<uint8_t> decoded_bits;
            int8_t guess_bit;

            const int n_samples_DELIM, n_samples_PW, n_samples_TRCAL, n_samples_RTCAL;

            bool check_length(int pulse_len, int expected_len, double tolerant_rate);
            void go_next_decode_state(void);

          public:
            reader_decoder(int n_samples_DELIM, int n_samples_PW, int n_samples_TRCAL, int n_samples_RTCAL);
            int up_pulse(int pulse_len);
            int down_pulse(int pulse_len);

            std::vector<uint8_t> get_bits(void);
            void set_preamble(void);
            void set_framesync(void);

            int reset(void);
        } * decoder;
      public:
        gate_impl(int sample_rate);
        ~gate_impl();

        void forecast (int noutput_items, gr_vector_int &ninput_items_required);

        int general_work(int noutput_items,
            gr_vector_int &ninput_items,
            gr_vector_const_void_star &input_items,
            gr_vector_void_star &output_items);

        void gate_fail();
    };
  } // namespace rfid
} // namespace gr

#endif /* INCLUDED_RFID_GATE_IMPL_H */
