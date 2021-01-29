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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gnuradio/io_signature.h>
#include "gate_impl.h"
#include <sys/time.h>
#include <stdio.h>

#define AMP_LOWBOUND (0.01) //this will let us find the lowest bound
#define MIN_PULSE (5)

#define AMP_POS_THRESHOLD_RATE (0.7)
#define AMP_NEG_THRESHOLD_RATE (0.3)

#define MAX_SEARCH_TRACK (MAX_SEARCH_TRACK_TIME * sample_rate)
#define MAX_SEARCH_TRACK_TIME (10e-3)
#define MAX_SEARCH_READY (MAX_SEARCH_READY_TIME * sample_rate)
#define MAX_SEARCH_READY_TIME (4e-3)
#define MAX_SEARCH_SEEK  (MAX_SEARCH_SEEK_TIME * sample_rate)
#define MAX_SEARCH_SEEK_TIME  (4e-3)
#define AMBIENT_START    (AMBIENT_START_TIME * sample_rate)
#define AMBIENT_START_TIME  (10e-3)

namespace gr
{
  namespace rfid
  {
    gate::sptr

      gate::make(int sample_rate)
      {
        return gnuradio::get_initial_sptr(new gate_impl(sample_rate));
      }

    /*
     * The private constructor
     */
    gate_impl::gate_impl(int sample_rate)
      : gr::block("gate",
          gr::io_signature::make(1, 1, sizeof(gr_complex)),
          gr::io_signature::make(1, 1, sizeof(gr_complex))),
      n_samples(0), avg_dc(0,0), num_pulses(0)
    {
      this->sample_rate  = sample_rate;
      n_samples_T1       = T1_D       * (sample_rate / pow(10,6));
      n_samples_TAG_BIT  = TPRI_D  * (sample_rate / pow(10,6));
      n_samples_PW       = PW_D  * (sample_rate / pow(10,6));
      n_samples_RTCAL    = RTCAL_D  * (sample_rate / pow(10,6));
      n_samples_TRCAL    = TRCAL_D  * (sample_rate / pow(10,6));
      n_samples_DELIM    = DELIM_D  * (sample_rate / pow(10,6));

      decoder = new reader_decoder(n_samples_DELIM, n_samples_PW, n_samples_TRCAL, n_samples_RTCAL);

      // First block to be scheduled
      initialize_reader_state();

    }

    /*
     * Our virtual destructor.
     */
    gate_impl::~gate_impl()
    {
      delete decoder;
    }

    void
      gate_impl::forecast (int noutput_items, gr_vector_int &ninput_items_required)
      {
        ninput_items_required[0] = noutput_items;
      }

    int
      gate_impl::general_work (int noutput_items,
          gr_vector_int &ninput_items,
          gr_vector_const_void_star &input_items,
          gr_vector_void_star &output_items)
      {
        const gr_complex *in = (const gr_complex *) input_items[0];
        gr_complex *out = (gr_complex *) output_items[0];

        int number_samples_consumed = ninput_items[0];
        int written = 0;


        log.open(log_file_path, std::ios::app);
        if(reader_state->gate_status != GATE_CLOSED)
        {
          for(int i=0 ; i<ninput_items[0] ; i++)
          {
            iq_count++;
            gr_complex sample = in[i];
            char data[8];
            memcpy(data, &sample, 8);

#ifdef __GATE_DEBUG__
            if(prev_gate_status != reader_state->gate_status){
              prev_gate_status = reader_state->gate_status;
              switch(reader_state->gate_status){
                case GATE_START:
                  log<<"gate start"<<std::endl;
                  break;
                case GATE_SEEK_RN16:
                  log<<"gate seek RN16"<<std::endl;
                  break;
                case GATE_TRACK:
                  log<<"gate track"<<std::endl;
                  break;
                case GATE_READY:
                  log<<"gate ready"<<std::endl;
                  break;  
                case GATE_SEEK:
                  log<<"gate seek"<<std::endl;
                  break;  
                case GATE_OPEN:
                  log<<"gate open"<<std::endl;
                  break;  
                case GATE_CLOSED:
                  log<<"gate closed"<<std::endl;
                  break;  
                default:
                  log<<"WHAT THE HELL???"<<std::endl;
              }
            }
#endif


            //gate at the beginning
            //
            //In here we do:
            //  - Skipping off-part at the beginning
            //  - calculate DC offset
            if(reader_state->gate_status == GATE_START)
            {
              if(++n_samples <= 20000) 
              {
                avg_dc += sample;
              }
              else if(n_samples > 26000)
              {
                avg_dc /= 20000;
                log << "n_samples_TAG_BIT= " << n_samples_TAG_BIT << std::endl;
                log << "Average of first 20000 amplitudes= " << avg_dc << std::endl;

                number_samples_consumed = i-1;

                avg_iq = gr_complex(0,0);
                n_samples = 0;
                amp_pos_threshold = 0;
                amp_neg_threshold = 0;
                max_count = MAX_SEARCH_SEEK;

                reader_state->gate_status = GATE_CLOSED;
                reader_state->gen2_logic_status = SEND_QUERY;

                break;
              }
            }
            //gate mode configure
            else if(reader_state->gate_status == GATE_SEEK_RN16)
            {
              log << "│ Gate seek RN16.." << std::endl;
              reader_state->n_samples_to_ungate = (RN16_BITS + TAG_PREAMBLE_BITS + EXTRA_BITS) * n_samples_TAG_BIT;
              reader_state->gate_status = GATE_SEEK;
              avg_iq = gr_complex(0,0);
              n_samples = 0;
              amp_pos_threshold = 0;
              amp_neg_threshold = 0;
              max_count = MAX_SEARCH_TRACK;
              gate_log_samples.clear();
            }
            else if(reader_state->gate_status == GATE_SEEK_EPC)
            {
              log << "│ Gate seek EPC.." << std::endl;
              reader_state->n_samples_to_ungate = (EPC_BITS + TAG_PREAMBLE_BITS + EXTRA_BITS) * n_samples_TAG_BIT;
              reader_state->gate_status = GATE_SEEK;
              avg_iq = gr_complex(0,0);
              n_samples = 0;
              amp_pos_threshold = 0;
              amp_neg_threshold = 0;
              max_count = MAX_SEARCH_TRACK;
            }

            //set reader decoder to decoder preambled version or framsync version
            if(reader_state->reader_sent_status == PREAMBLE)
              decoder->set_preamble();
            else if(reader_state->reader_sent_status == FRAME_SYNC)
              decoder->set_framesync();

            sample -= avg_dc;
            gate_log_samples.push_back(sample);

            //start gating

            //Calculating Average IQ
            if(reader_state->gate_status == GATE_SEEK)
            {
              n_samples++;  //count processed sample number of this mode

              if(--max_count <= 0){
                gate_fail();
                number_samples_consumed = i-1;
                break;
              }else if(n_samples < (int)(n_samples_T1 * 0.4)){
                //add for average iq amplitude
                avg_iq += sample;
              }else if(n_samples == (int)(n_samples_T1 * 0.4)){
                //get average iq amplitude in here
                avg_iq /= n_samples;
                log << "| AVG amp : " <<avg_iq<<std::endl;
                log << "| FIND first neg amp"<<std::endl;

                amp_pos_threshold = abs(avg_iq) * AMP_POS_THRESHOLD_RATE;
                amp_neg_threshold = abs(avg_iq) * AMP_NEG_THRESHOLD_RATE;

                reader_state->gate_status = GATE_TRACK;

                signal_state = POS_EDGE;
                num_pulses = 0;
                n_samples = 0;
              }
            }
            else if(reader_state->gate_status == GATE_TRACK)
            {
              n_samples++;  //count processed sample number of this mode

              if(--max_count <= 0)
              {//log<<std::endl;
                log<<"GATE TRACK"<<std::endl;
                log<<"abs value : "<<abs(sample)<<std::endl;

                if(signal_state == POS_EDGE) log<<"signal_state : POS_EDGE"<<std::endl;
                else if(signal_state == NEG_EDGE)  log<<"signal_state : NEG_EDGE"<<std::endl;
                log<<"num pulse : "<<num_pulses<<std::endl;
                gate_fail();
                number_samples_consumed = i-1;
                break;
              }//og<<sample<<" ";
              if((signal_state == NEG_EDGE) && (abs(sample) > amp_pos_threshold))
              {
                int bit_num = decoder->down_pulse(n_samples);
                //if we decode bits as much as we needed
                if(bit_num == reader_state->sent_bit.size())
                {
                  if(decoder->get_bits() != reader_state->sent_bit) //if decode failed go back to GATE_SEEK
                    reader_state->gate_status = GATE_SEEK;
                  else  //if we successfully decode, go to GATE_READY
                  {
                    reader_state->gate_status = GATE_READY;
                    max_count = MAX_SEARCH_READY;
                    n_samples = 0;
                  }
                }

                signal_state = POS_EDGE;
                n_samples = 0;
              }
              else if((signal_state == POS_EDGE) && (abs(sample) < amp_neg_threshold))
              {
                decoder->up_pulse(n_samples);

                signal_state = NEG_EDGE;
                n_samples = 0;
              }
            }
            else if(reader_state->gate_status == GATE_READY)
            {
              if(--max_count <= 0)
              {//log<<std::endl;
                log<<"GATE READY"<<std::endl;
                gate_fail();
                number_samples_consumed = i-1;
                break;
              }//log<<sample<<" ";
              if(signal_state == POS_EDGE){ 
                if(abs(sample) < amp_neg_threshold){
                  signal_state = NEG_EDGE;
                }else if(n_samples++ > (int)n_samples_T1/2)
                {//log<<std::endl;
                  log << "│ Gate open! " << n_samples<<", "<<gate_log_samples.size() << std::endl;
                  log << "├──────────────────────────────────────────────────" << std::endl;
                  reader_state->gate_status = GATE_OPEN;
                  written = 0;
                  n_samples = 0;
                  continue;
                }
              }else if((signal_state == NEG_EDGE) && (abs(sample) > amp_pos_threshold))
              {
                n_samples = 0;
                signal_state = POS_EDGE;
              }
            }
            else if(reader_state->gate_status == GATE_OPEN)
            {
              if(++n_samples > reader_state->n_samples_to_ungate)
              {
                gateLogSave();          
                number_samples_consumed = i-1;
                n_samples = 0;
                reader_state->gate_status = GATE_CLOSED;
                break;
              }
              out[written++] = sample;
            }
          }
        } //end of "gate_status != GATE_CLOSE"
        else
          iq_count += number_samples_consumed;

        log.close();

        consume_each(number_samples_consumed);
        return written;
      }

    void gate_impl::gate_fail(void)
    {
      log << "│ Gate search FAIL!" << std::endl;

      log<<"| location : "<<iq_count<<std::endl;
      std::cout << "Gate FAIL!!";
      reader_state->gate_status = GATE_CLOSED;

      gateLogSave();
      decoder->reset();

      reader_state->reader_stats.cur_slot_number++;
      if(reader_state->reader_stats.cur_slot_number > reader_state->reader_stats.max_slot_number)
      {
        reader_state->reader_stats.cur_inventory_round ++;
        reader_state->reader_stats.cur_slot_number = 1;

        log << "└──────────────────────────────────────────────────" << std::endl;
        if(reader_state->reader_stats.cur_inventory_round > MAX_NUM_QUERIES)
        {
          reader_state->reader_stats.cur_inventory_round--;
          reader_state->decoder_status = DECODER_TERMINATED;
        }
        else reader_state->gen2_logic_status = SEND_QUERY;
      }
      else
      {
        log << "├──────────────────────────────────────────────────" << std::endl;
        reader_state->gen2_logic_status = SEND_QUERY_REP;
      }

    }

    void gate_impl::gateLogSave(void){
      std::ofstream gate_logger;

      gate_logger.open("gateOpenTracker/"+std::to_string(reader_state->reader_stats.cur_inventory_round), std::ios::out|std::ios::binary);
      for(int i = 0; i<gate_log_samples.size();i++){
        gate_logger.write((char*)&gate_log_samples[i], sizeof(gr_complex));
      }

      gate_log_samples.clear();
      gate_logger.close();
    }
  } // namespace rfid
} // namespace gr
