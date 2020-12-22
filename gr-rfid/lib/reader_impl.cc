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
#include "reader_impl.h"
#include "rfid/global_vars.h"
#include <sys/time.h>

// polynomial crc16-ccitt
static const uint16_t crc16Table[256]= {
  0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50a5,
  0x60c6, 0x70e7, 0x8108, 0x9129, 0xa14a, 0xb16b,
  0xc18c, 0xd1ad, 0xe1ce, 0xf1ef, 0x1231, 0x0210,
  0x3273, 0x2252, 0x52b5, 0x4294, 0x72f7, 0x62d6,
  0x9339, 0x8318, 0xb37b, 0xa35a, 0xd3bd, 0xc39c,
  0xf3ff, 0xe3de, 0x2462, 0x3443, 0x0420, 0x1401,
  0x64e6, 0x74c7, 0x44a4, 0x5485, 0xa56a, 0xb54b,
  0x8528, 0x9509, 0xe5ee, 0xf5cf, 0xc5ac, 0xd58d,
  0x3653, 0x2672, 0x1611, 0x0630, 0x76d7, 0x66f6,
  0x5695, 0x46b4, 0xb75b, 0xa77a, 0x9719, 0x8738,
  0xf7df, 0xe7fe, 0xd79d, 0xc7bc, 0x48c4, 0x58e5,
  0x6886, 0x78a7, 0x0840, 0x1861, 0x2802, 0x3823,
  0xc9cc, 0xd9ed, 0xe98e, 0xf9af, 0x8948, 0x9969,
  0xa90a, 0xb92b, 0x5af5, 0x4ad4, 0x7ab7, 0x6a96,
  0x1a71, 0x0a50, 0x3a33, 0x2a12, 0xdbfd, 0xcbdc,
  0xfbbf, 0xeb9e, 0x9b79, 0x8b58, 0xbb3b, 0xab1a,
  0x6ca6, 0x7c87, 0x4ce4, 0x5cc5, 0x2c22, 0x3c03,
  0x0c60, 0x1c41, 0xedae, 0xfd8f, 0xcdec, 0xddcd,
  0xad2a, 0xbd0b, 0x8d68, 0x9d49, 0x7e97, 0x6eb6,
  0x5ed5, 0x4ef4, 0x3e13, 0x2e32, 0x1e51, 0x0e70,
  0xff9f, 0xefbe, 0xdfdd, 0xcffc, 0xbf1b, 0xaf3a,
  0x9f59, 0x8f78, 0x9188, 0x81a9, 0xb1ca, 0xa1eb,
  0xd10c, 0xc12d, 0xf14e, 0xe16f, 0x1080, 0x00a1,
  0x30c2, 0x20e3, 0x5004, 0x4025, 0x7046, 0x6067,
  0x83b9, 0x9398, 0xa3fb, 0xb3da, 0xc33d, 0xd31c,
  0xe37f, 0xf35e, 0x02b1, 0x1290, 0x22f3, 0x32d2,
  0x4235, 0x5214, 0x6277, 0x7256, 0xb5ea, 0xa5cb,
  0x95a8, 0x8589, 0xf56e, 0xe54f, 0xd52c, 0xc50d,
  0x34e2, 0x24c3, 0x14a0, 0x0481, 0x7466, 0x6447,
  0x5424, 0x4405, 0xa7db, 0xb7fa, 0x8799, 0x97b8,
  0xe75f, 0xf77e, 0xc71d, 0xd73c, 0x26d3, 0x36f2,
  0x0691, 0x16b0, 0x6657, 0x7676, 0x4615, 0x5634,
  0xd94c, 0xc96d, 0xf90e, 0xe92f, 0x99c8, 0x89e9,
  0xb98a, 0xa9ab, 0x5844, 0x4865, 0x7806, 0x6827,
  0x18c0, 0x08e1, 0x3882, 0x28a3, 0xcb7d, 0xdb5c,
  0xeb3f, 0xfb1e, 0x8bf9, 0x9bd8, 0xabbb, 0xbb9a,
  0x4a75, 0x5a54, 0x6a37, 0x7a16, 0x0af1, 0x1ad0,
  0x2ab3, 0x3a92, 0xfd2e, 0xed0f, 0xdd6c, 0xcd4d,
  0xbdaa, 0xad8b, 0x9de8, 0x8dc9, 0x7c26, 0x6c07,
  0x5c64, 0x4c45, 0x3ca2, 0x2c83, 0x1ce0, 0x0cc1,
  0xef1f, 0xff3e, 0xcf5d, 0xdf7c, 0xaf9b, 0xbfba,
  0x8fd9, 0x9ff8, 0x6e17, 0x7e36, 0x4e55, 0x5e74,
  0x2e93, 0x3eb2, 0x0ed1, 0x1ef0
};

namespace gr
{
  namespace rfid
  {
    reader::sptr
    reader::make(int sample_rate, int dac_rate)
    {
      return gnuradio::get_initial_sptr
      (new reader_impl(sample_rate,dac_rate));
    }

    /*
    * The private constructor
    */
    reader_impl::reader_impl(int sample_rate, int dac_rate)
    : gr::block("reader",
      gr::io_signature::make( 1, 1, sizeof(float)),
      gr::io_signature::make( 1, 1, sizeof(float)))
    {
      sample_d = 1.0/dac_rate * pow(10,6);

      // Number of samples for transmitting
      n_data0_s = 2 * PW_D / sample_d;
      n_data1_s = 4 * PW_D / sample_d;
      n_pw_s    = PW_D    / sample_d;
      n_cw_s    = CW_D    / sample_d;
      n_delim_s = DELIM_D / sample_d;
      n_trcal_s = TRCAL_D / sample_d;

      // CW waveforms of different sizes
      n_cwquery_s   = (T1_D+T2_D+RN16_D)/sample_d;     //RN16
      n_cwack_s     = (T1_D+T2_D+EPC_D)/sample_d;    //EPC   if it is longer than nominal it wont cause tags to change inventoried flag
      n_p_down_s     = (P_DOWN_D)/sample_d;

      p_down.resize(n_p_down_s);        // Power down samples
      cw_query.resize(n_cwquery_s);      // Sent after query/query rep
      cw_ack.resize(n_cwack_s);          // Sent after ack

      std::fill_n(cw_query.begin(), cw_query.size(), 1);
      std::fill_n(cw_ack.begin(), cw_ack.size(), 1);

      // Construct vectors (resize() default initialization is zero)
      data_0.resize(n_data0_s);
      data_1.resize(n_data1_s);
      cw.resize(n_cw_s);
      delim.resize(n_delim_s);
      rtcal.resize(n_data0_s + n_data1_s);
      trcal.resize(n_trcal_s);

      // Fill vectors with data
      std::fill_n(data_0.begin(), data_0.size()/2, 1);
      std::fill_n(data_1.begin(), 3*data_1.size()/4, 1);
      std::fill_n(cw.begin(), cw.size(), 1);
      std::fill_n(rtcal.begin(), rtcal.size() - n_pw_s, 1); // RTcal
      std::fill_n(trcal.begin(), trcal.size() - n_pw_s, 1); // TRcal

      // create preamble
      preamble.insert( preamble.end(), delim.begin(), delim.end() );
      preamble.insert( preamble.end(), data_0.begin(), data_0.end() );
      preamble.insert( preamble.end(), rtcal.begin(), rtcal.end() );
      preamble.insert( preamble.end(), trcal.begin(), trcal.end() );

      // create framesync
      frame_sync.insert( frame_sync.end(), delim.begin() , delim.end() );
      frame_sync.insert( frame_sync.end(), data_0.begin(), data_0.end() );
      frame_sync.insert( frame_sync.end(), rtcal.begin() , rtcal.end() );

      // create query rep
      query_rep.insert( query_rep.end(), frame_sync.begin(), frame_sync.end());
      query_rep.insert( query_rep.end(), data_0.begin(), data_0.end() );
      query_rep.insert( query_rep.end(), data_0.begin(), data_0.end() );
      query_rep.insert( query_rep.end(), data_0.begin(), data_0.end() );
      query_rep.insert( query_rep.end(), data_0.begin(), data_0.end() );

      // create nak
      nak.insert( nak.end(), frame_sync.begin(), frame_sync.end());
      nak.insert( nak.end(), data_1.begin(), data_1.end() );
      nak.insert( nak.end(), data_1.begin(), data_1.end() );
      nak.insert( nak.end(), data_0.begin(), data_0.end() );
      nak.insert( nak.end(), data_0.begin(), data_0.end() );
      nak.insert( nak.end(), data_0.begin(), data_0.end() );
      nak.insert( nak.end(), data_0.begin(), data_0.end() );
      nak.insert( nak.end(), data_0.begin(), data_0.end() );
      nak.insert( nak.end(), data_0.begin(), data_0.end() );

      gen_query_bits();
      gen_query_adjust_bits();

      use_random_rn = true;
      //use_random_rn = false;

      // random round number
      if(use_random_rn == true)
      {
        srand(time(NULL));
        std::ofstream debug("RN", std::ios::app);

        for(int i=0 ; i<MAX_NUM_QUERIES ; i++)
        {
          int rn = rand() % (MAX_POSSIBLE_ROUND + 1);
          if(i == 0 || find(random_rn_list.begin(), random_rn_list.end(), rn) == random_rn_list.end())
          {
            random_rn_list.push_back(rn);
            debug << rn << " ";
          }
          else i--;  // retry
        }

        debug << std::endl;
        debug.close();
      }
    }

    void reader_impl::gen_query_bits()
    {
      int num_ones = 0, num_zeros = 0;

      query_bits.resize(0);
      query_bits.insert(query_bits.end(), &QUERY_CODE[0], &QUERY_CODE[4]);

      query_bits.push_back(DR);
      query_bits.insert(query_bits.end(), &M[0], &M[2]);
      query_bits.push_back(TREXT);
      query_bits.insert(query_bits.end(), &SEL[0], &SEL[2]);
      query_bits.insert(query_bits.end(), &SESSION[0], &SESSION[2]);
      query_bits.push_back(TARGET);

      query_bits.insert(query_bits.end(), &Q_VALUE[FIXED_Q][0], &Q_VALUE[FIXED_Q][4]);
      crc_append(query_bits);
    }

    void reader_impl::gen_ack_bits(const float * in)
    {
      ack_bits.resize(0);
      ack_bits.insert(ack_bits.end(), &ACK_CODE[0], &ACK_CODE[2]);
      ack_bits.insert(ack_bits.end(), &in[0], &in[16]);
    }

    void reader_impl::gen_query_adjust_bits()
    {
      query_adjust_bits.resize(0);
      query_adjust_bits.insert(query_adjust_bits.end(), &QADJ_CODE[0], &QADJ_CODE[4]);
      query_adjust_bits.insert(query_adjust_bits.end(), &SESSION[0], &SESSION[2]);
      query_adjust_bits.insert(query_adjust_bits.end(), &Q_UPDN[1][0], &Q_UPDN[1][3]);
    }

    reader_impl::~reader_impl(){}

    void reader_impl::forecast(int noutput_items, gr_vector_int &ninput_items_required)
    {
      ninput_items_required[0] = 0;
    }

    void reader_impl::transmit(float* out, int* written, std::vector<float> bits)
    {
      memcpy(&out[*written], &bits[0], sizeof(float) * bits.size());
      (*written) += bits.size();
    }

    void reader_impl::transmit_bits(float* out, int* written, std::vector<float> bits)
    {
      for(int i=0 ; i<bits.size() ; i++)
      {
        if(bits[i] == 1) transmit(out, written, data_1);
        else transmit(out, written, data_0);
      }
    }

    int reader_impl::general_work(int noutput_items, gr_vector_int &ninput_items, gr_vector_const_void_star &input_items, gr_vector_void_star &output_items)
    {
      const float* in = (const float*)input_items[0];
      float* out = (float*)output_items[0];
      int consumed = 0;
      int written = 0;

      if(reader_state->gen2_logic_status != IDLE)
      {
        reader_log ys;

        if(reader_state->gen2_logic_status == START)
        {
          transmit(out, &written, cw_ack);
          reader_state->gen2_logic_status = IDLE;
          ys.makeLog_init(n_delim_s, n_data0_s, n_data1_s, n_trcal_s, n_cwquery_s, n_cwack_s, sample_d);
        }
        else if(reader_state->gen2_logic_status == SEND_QUERY)
        {
          ys.makeLog_query(false);
          int cur_round;
          if(use_random_rn == true)
          {
            cur_round = random_rn_list[reader_state->reader_stats.cur_inventory_round - 1];
            std::cout << std::endl << "[" << reader_state->reader_stats.cur_inventory_round << "_" << cur_round << "]\t";
          }
          else
          {
            cur_round = reader_state->reader_stats.cur_inventory_round;
            std::cout << std::endl << "[" << reader_state->reader_stats.cur_inventory_round << "]\t";
          }
          reader_state->reader_stats.n_queries_sent +=1;

          // Controls the other two blocks
          reader_state->decoder_status = DECODER_DECODE_EPC;
          reader_state->gate_status    = GATE_SEEK_EPC;

          transmit(out, &written, cw);
          transmit(out, &written, preamble);

	        std::vector<int> my_query;
          my_query.push_back(1);
          my_query.push_back(0);
          my_query.push_back(0);
          my_query.push_back(0);  // query command (4-bits)
          my_query.push_back(0);
          my_query.push_back(0);
          my_query.push_back(0);
          my_query.push_back(0);  // dummy (4-bits)

          std::vector<int> round_bin;
          while(cur_round)
          {
            if(cur_round % 2) round_bin.push_back(1);
            else round_bin.push_back(0);

            cur_round /= 2;
          }

          while(round_bin.size() != 16)
            round_bin.push_back(0);

          for(int i=15 ; i>=0 ; i--)
          {
            if(round_bin[i]) my_query.push_back(1);
            else my_query.push_back(0);
          }

          for(int i=0 ; i<96 ; i++)
          {
            if(rand() % 2) my_query.push_back(0);
            else my_query.push_back(1);
          }

          unsigned char crc_base[14];
          for(int i=0 ; i<14 ; i++)
          {
            unsigned char byte = 0;
            for(int j=0 ; j<8 ; j++)
            {
              byte *= 2;
              if(my_query[8+i*8+j]) byte++;
            }
            crc_base[i] = byte;
          }

          unsigned short crc_value = 0xffff;
          for(int i=0 ; i<14 ; i++)
            crc_value = (crc_value << 8) ^ crc16Table[((crc_value >> 8) ^ crc_base[i]) & 0x00ff];

          std::vector<int> crc_bin;
          while(crc_value)
          {
            if(crc_value % 2) crc_bin.push_back(1);
            else crc_bin.push_back(0);

            crc_value /= 2;
          }

          while(crc_bin.size() != 16)
            crc_bin.push_back(0);

          for(int i=15 ; i>=0 ; i--)
          {
            if(crc_bin[i]) my_query.push_back(1);
            else my_query.push_back(0);
          }

          std::ofstream debug("label", std::ios::app);
          for(int i=8 ; i<my_query.size() ; i++)  // do not backup command 8-bits
            debug << my_query[i];
          debug << std::endl;
          debug.close();

          for(int i=0 ; i<my_query.size() ; i++)
          {
            if(my_query[i] == 0) transmit(out, &written, data_0);
            else transmit(out, &written, data_1);
          }

          std::cout << "Send packet | ";
          reader_state->gen2_logic_status = SEND_CW;
        }
        else if(reader_state->gen2_logic_status == SEND_CW)
        {
          for(int i=0 ; i<T1_D+EPC_D+T2_D ; i++) out[written++] = 1;
          reader_state->gen2_logic_status = IDLE;
        }
      }

      consume_each(consumed);
      return written;
    }

    int reader_impl::calc_usec(const struct timeval start, const struct timeval end)
    {
      int sec = end.tv_sec - start.tv_sec;
      int usec = sec * 1e6;
      return usec + end.tv_usec - start.tv_usec;
    }

    void reader_impl::print_results()
    {
      std::ofstream result(result_file_path, std::ios::out);

      result << std::endl << "┌──────────────────────────────────────────────────" << std::endl;
      result << "│ Number of QUERY/QUERYREP sent: " << reader_state->reader_stats.n_queries_sent << std::endl;
      result << "│ Number of ACK sent: " << reader_state->reader_stats.n_ack_sent << std::endl;
      result << "│ ";
      for(int i=0 ; i<reader_state->reader_stats.ack_sent.size() ; i++)
        result << reader_state->reader_stats.ack_sent[i] << " ";
      result << std::endl << "│ Current Inventory round: " << reader_state->reader_stats.cur_inventory_round << std::endl;
      result << "├──────────────────────────────────────────────────" << std::endl;
      result << "│ Number of gate fail: " << reader_state->reader_stats.n_gate_fail << std::endl;
      result << "│ Number of correctly decoded EPC: " << reader_state->reader_stats.n_epc_correct << std::endl;
      result << "│ Number of unique tags: " << reader_state->reader_stats.tag_reads.size() << std::endl;

      if(reader_state->reader_stats.tag_reads.size())
      {
        result << "├───────────────┬──────────────────────────────────" << std::endl;
        result << "│ Tag ID\t│ Num of reads" << std::endl;
        result << "├───────────────┼──────────────────────────────────" << std::endl;
      }

      std::map<int,int>::iterator it;
      for(it = reader_state->reader_stats.tag_reads.begin(); it != reader_state->reader_stats.tag_reads.end(); it++)
        result << "│ " << it->first << "\t\t" << "│ " << it->second << std::endl;

      if(reader_state->reader_stats.tag_reads.size())
        result << "├───────────────┴──────────────────────────────────" << std::endl;
      else
        result << "├──────────────────────────────────────────────────" << std::endl;

      gettimeofday (&reader_state-> reader_stats.end, NULL);
      int execution_time = calc_usec(reader_state->reader_stats.start, reader_state->reader_stats.end);
      result << "│ Execution time: " << execution_time << " (μs)" << std::endl;
      result << "│ Throughput(EPC): " << (double)reader_state->reader_stats.n_epc_correct * (EPC_BITS - 1) / execution_time * 1e6 << " (bits/second)" << std::endl;
      result << "└──────────────────────────────────────────────────" << std::endl;

      result.close();
    }

    /* Function adapted from https://www.cgran.org/wiki/Gen2 */
    void reader_impl::crc_append(std::vector<float> & q)
    {
      int crc[] = {1,0,0,1,0};

      for(int i = 0; i < 17; i++)
      {
        int tmp[] = {0,0,0,0,0};
        tmp[4] = crc[3];
        if(crc[4] == 1)
        {
          if (q[i] == 1)
          {
            tmp[0] = 0;
            tmp[1] = crc[0];
            tmp[2] = crc[1];
            tmp[3] = crc[2];
          }
          else
          {
            tmp[0] = 1;
            tmp[1] = crc[0];
            tmp[2] = crc[1];
            if(crc[2] == 1)
            {
              tmp[3] = 0;
            }
            else
            {
              tmp[3] = 1;
            }
          }
        }
        else
        {
          if (q[i] == 1)
          {
            tmp[0] = 1;
            tmp[1] = crc[0];
            tmp[2] = crc[1];
            if(crc[2] == 1)
            {
              tmp[3] = 0;
            }
            else
            {
              tmp[3] = 1;
            }
          }
          else
          {
            tmp[0] = 0;
            tmp[1] = crc[0];
            tmp[2] = crc[1];
            tmp[3] = crc[2];
          }
        }
        memcpy(crc, tmp, 5*sizeof(float));
      }
      for (int i = 4; i >= 0; i--)
        q.push_back(crc[i]);
    }
  }
}
