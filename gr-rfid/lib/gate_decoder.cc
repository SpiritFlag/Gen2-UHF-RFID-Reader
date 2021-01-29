#include "gate_impl.h"

namespace gr{
  namespace rfid{
    gate_impl::reader_decoder::reader_decoder
      (int n_samples_DELIM, int n_samples_PW, int n_samples_TRCAL, int n_samples_RTCAL)
      :n_samples_DELIM(n_samples_DELIM), n_samples_PW(n_samples_PW), n_samples_TRCAL(n_samples_TRCAL), n_samples_RTCAL(n_samples_RTCAL), guess_bit(-1)
      {
        decoded_bits.resize(23);
        reset();
      }

    bool
      gate_impl::reader_decoder::check_length(int pulse_len, int expected_len, double tolerant_rate)
      {
        if(((1.0 - tolerant_rate)*expected_len <= pulse_len) && (pulse_len <= (1.0 + tolerant_rate)*expected_len))
          return true;
        else
          return false;
      }

    void
      gate_impl::reader_decoder::go_next_decode_state(void)
      {
        switch(decode_state){
          case DELIMITER:
            decode_state = DATA0;
            break;
          case DATA0:
            decode_state = RTCAL;
            break;
          case RTCAL:
            if(is_preamble) decode_state = TRCAL; //preamble
            else            decode_state = DATAS; //frame sync
            break;
          case TRCAL:
            decode_state = DATAS;
            break;
          default:
            ;
        }

      }

    int 
      gate_impl::reader_decoder::up_pulse(int pulse_len)
      {
        if(up_down_state == true)
        {
          up_down_state = false;
          int expected_len = 0;

          //Handling up pulse based on pulse_len
          //state DATAS means we are now decoding real bits
          //state DELIMITER means nothing here, we need to skip it.
          //other states handles with expected len
          switch(decode_state)
          {
            case DATAS:
              if(check_length(pulse_len, n_samples_PW, 0.1))
                guess_bit = 0;
              else if(check_length(pulse_len, n_samples_PW * 3, 0.1))
                guess_bit = 1;
              else
                reset();
              return decoded_bits.size();
            case DELIMITER:
              reset();
              return decoded_bits.size();

            case DATA0:
              expected_len = n_samples_PW;
              break;
            case RTCAL:
              expected_len = n_samples_RTCAL - n_samples_PW;
              break;
            case TRCAL:
              expected_len = n_samples_TRCAL - n_samples_PW;
              break;
            default:
              ; //do nothing

          }

          if(!check_length(pulse_len, expected_len, 0.1))
            reset();
        }
        else
        {
          reset();
          return decoded_bits.size();
        }
      }

    int 
      gate_impl::reader_decoder::down_pulse(int pulse_len)
      {
        if(up_down_state == false)
        {
          up_down_state = true;
          int expected_len = 0;

          //Handling up pulse based on pulse_len
          //state DATAS means we are now decoding real bits
          //state DELIMITER means nothing here, we need to skip it.
          //other states handles with expected len
          switch(decode_state)
          {
            case DATAS:
              if(check_length(pulse_len, n_samples_PW, 0.1))
                decoded_bits.push_back(guess_bit);
              else
                reset();
              return decoded_bits.size();
            case DELIMITER:
              expected_len = n_samples_DELIM;
              break;
            case DATA0:
              expected_len = n_samples_PW;
              break;
            case RTCAL:
              expected_len = n_samples_PW;
              break;
            case TRCAL:
              expected_len = n_samples_PW;
              break;
            default:
              ; //do nothing

          }

          if(check_length(pulse_len, expected_len, 0.1))
          {
            //check success, go to next decode state
            go_next_decode_state();
          }
          else
          {
            if(decode_state == DELIMITER)
            {
              //if we are looking for Delimiter, then we quit here
              reset();
            }else
            {
              reset();

              //we check this down pulse again by recursive method
              up_down_state = false;
              down_pulse(pulse_len);
            }
          }

        }
        else
        {
          reset();
        }

        return decoded_bits.size();
      }

    std::vector<uint8_t> gate_impl::reader_decoder::get_bits(void) {return decoded_bits;}

    void gate_impl::reader_decoder::set_preamble(void)  {is_preamble = true;}
    void gate_impl::reader_decoder::set_framesync(void)  {is_preamble = false;}

    int 
      gate_impl::reader_decoder::reset(void)
      {
        guess_bit = -1;
        up_down_state = false;
        decode_state = DELIMITER;
        decoded_bits.clear();
      }
  }//end of gr
}//end of rfid
