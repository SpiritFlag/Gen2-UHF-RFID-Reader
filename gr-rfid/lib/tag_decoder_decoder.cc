/* -*- c++ -*- */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "tag_decoder_impl.h"
#include <cmath>

#define SHIFT_SIZE 5  // used in tag_detection

#define FM0_MASKS_LENGTH  4
#define TAG_PREAMBLE_MASKS_LENGTH  12


//#define __DEBUG__

namespace gr
{
  namespace rfid
  {

    const static float FM0_MASKS[2][FM0_MASKS_LENGTH] = { // first, last elements are extra bits. second, third elements are real signal.
      {1, -1, 1, -1}, {1, -1, -1, 1}, // low level start
    };


    // FM0 encoding preamble sequences
    // const int TAG_PREAMBLE[] = {1,1,-1,1,-1,-1,1,-1,-1,-1,1,1};
    const static float TAG_PREAMBLE_MASKS[TAG_PREAMBLE_MASKS_LENGTH] =
    {1, 1, -1, 1, -1, -1, 1, -1, -1, -1, 1, 1};


    int tag_decoder_impl::tag_sync(sample_information* ys)
      // This method searches the preamble and returns the start index of the tag data.
      // If the correlation value exceeds the threshold, it returns the start index of the tag data.
      // Else, it returns -1.
      // Threshold is an experimental value, so you might change this value within your environment.
    {
      int win_size = n_samples_TAG_BIT * TAG_PREAMBLE_BITS;
      float threshold = n_samples_TAG_BIT * 4;  // threshold verifing correlation value

      gr_complex corr_temp(0.0f,0.0f);
      gr_complex max_corr = 0.0f;
      int max_index = 0;
      gr_complex max_stddev = 0.0;

      // compare all samples with sliding except T1
      for(int i=0 ; i<ys->total_size()-win_size ; i++)  // i: start point
      {

        // calculate correlation value
        if(i==0){
          corr_temp = mask_correlation(ys, TAG_PREAMBLE_MASKS, TAG_PREAMBLE_MASKS_LENGTH, i , 1);
        }else{
          corr_temp = mask_shift_one_sample(ys,TAG_PREAMBLE_MASKS, TAG_PREAMBLE_MASKS_LENGTH, corr_temp, i, 1);
        }

        // get max correlation value for ith start point
        std::complex<double> corr = corr_temp;

        // compare with current max correlation value
        if(std::abs(corr) > std::abs(max_corr))
        {
          // calculate average_amp (threshold)
          std::complex<float> average_amp(0.0,0.0);
          for(int j=0 ; j<win_size ; j++)
            average_amp += ys->in(i+j);
          average_amp /= win_size;

          // calculate normalize_factor
          std::complex<float> standard_deviation(0.0,0.0);
          for(int j=0 ; j<win_size ; j++){
            standard_deviation.real(standard_deviation.real() + pow(ys->in(i+j).real() - average_amp.real(), 2));
            standard_deviation.imag(standard_deviation.imag() + pow(ys->in(i+j).imag() - average_amp.imag(), 2));

          }

          standard_deviation /= win_size;
          standard_deviation.real(pow(standard_deviation.real(),0.5));
          standard_deviation.imag(pow(standard_deviation.imag(),0.5));

          max_corr = corr;
          max_index = i;
          max_stddev = standard_deviation;
        }
      }


#ifdef __DEBUG__
      debug_log << "threshold= " << threshold << std::endl;
      debug_log << "corr= " << max_corr << std::endl;
      debug_log << "preamble index= " << max_index << std::endl;
      debug_log << "sample index= " << max_index + win_size << std::endl;
#endif

      max_corr.real(max_corr.real()/max_stddev.real());
      max_corr.imag(max_corr.imag()/max_stddev.imag());

      // check if correlation value exceeds threshold
      if((std::abs(max_corr)) > threshold) return max_index + win_size;
      else return -1;
    }

    static int correct_bit = 0;

    std::vector<float> tag_decoder_impl::tag_detection(sample_information* ys, int index, int n_expected_bit)
      // This method decodes n_expected_bit of data by using previous methods, and returns the vector of the decoded data.
      // index: start point of "data bit", do not decrease half bit!
    {
      std::vector<float> decoded_bits;


      int mask_level = 1;
      int shift = 0;
      double max_corr_sum = 0.0f;
      gr_complex max_complex_corr_sum(0.0, 0.0);

      for(int i=0 ; i<n_expected_bit ; i++)
      {
        int idx = index + i*n_samples_TAG_BIT + shift - n_samples_TAG_BIT/2;  // start point of decoding bit with shifting
        std::complex<double> max_corr(0,0);
        int max_bit;
        int curr_shift;


        //culculate left most shifted correlation value
        std::vector<std::complex<double>> corr_result(2), result(2);

        //calculate new corr value by shifting left to right one sample each
        for(int j=-SHIFT_SIZE ; j<=SHIFT_SIZE ; j++)
        {
          if(j == -SHIFT_SIZE){
            corr_result[0] = mask_correlation(ys, FM0_MASKS[0], FM0_MASKS_LENGTH, idx+j, mask_level);
            corr_result[1] = mask_correlation(ys, FM0_MASKS[1], FM0_MASKS_LENGTH, idx+j, mask_level);
          }else{
            corr_result[0] = mask_shift_one_sample(ys, FM0_MASKS[0], FM0_MASKS_LENGTH, corr_result[0], idx+j, mask_level);
            corr_result[1] = mask_shift_one_sample(ys, FM0_MASKS[1], FM0_MASKS_LENGTH, corr_result[1], idx+j, mask_level);
          }

          //Find the Biggest Correlation value
          for(int k=0; k<=1; k++){
            if(std::abs(corr_result[k]) > std::abs(max_corr)){
              max_corr = corr_result[k];
              result[0] = corr_result[0];
              result[1] = corr_result[1];
              max_bit = k;
              curr_shift = j;
            }
          }
        }

        max_corr_sum += std::abs(max_corr);
        max_complex_corr_sum += max_corr;


#ifdef __DEBUG__
        debug_log << "[" << i+1 << "th bit]\tcorr=";
        debug_log << std::left << std::setw(8) << max_corr;
        debug_log << "\tcurr_shift=" << curr_shift << "\tshift=";
        debug_log << std::left << std::setw(5) << shift;
        debug_log << "\tdecoded_bit=" << max_index;
        if(mask_level) debug_log << " (high start)" << std::endl;
        else debug_log << " (low start)" << std::endl;
#endif

        if(max_bit == 1){
          mask_level *= -1; // change mask_level(start level of the next bit) when the decoded bit is 1
        }

        decoded_bits.push_back(max_bit);
        shift += curr_shift;  // update the shift value
      }

      ys->set_corr(max_corr_sum/n_expected_bit);
      ys->set_complex_corr(max_complex_corr_sum/(float)n_expected_bit);


      int data = 0;
      for(int i = 0; i<16; i++){
        data = data << 1;
        data -= data & 1;
        if(decoded_bits[i] > 0.5){
          data += 1;
        }        //std::cout<<in[i];
      }

      if(data == 0xAAAA)
        correct_bit++;
      else{
        data = (data ^ 0xAAAA);
        int b_c = 0;
        int data1 = data;
        for(int i = 0; i<16; i++){
          if(data1 % 2 == 1)
            b_c++;
          data1 /= 2;
        }
        std::cout << std::hex<<data<<std::dec<<", "<<b_c<<" | ";

      }

      std::cout<<correct_bit<<" | ";

      return decoded_bits;
    }


    std::complex<double> tag_decoder_impl::mask_correlation(sample_information * ys, const float mask_data[],const int mask_length, int index, int mask_level){

      std::complex<double> result(0.0,0.0);

      for(int i_mask = 0; i_mask < mask_length; i_mask++){
        for(int i_sam = ((int)n_samples_TAG_BIT/2 * i_mask); i_sam < ((int)n_samples_TAG_BIT/2 * (i_mask + 1)); i_sam++){
          result += (ys->in(index + i_sam) * mask_data[i_mask]);
        }
      }

      result *= mask_level;

      return result;
    }

    std::complex<double> tag_decoder_impl::mask_shift_one_sample(sample_information * ys, const float mask_data[], const int mask_length, std::complex<double> prev_result, int index, int mask_level){
      int prev_index = index - 1;

      std::complex<double> result(0.0,0.0);

      result -= (ys->in(prev_index) * mask_data[0]);
      result += (ys->in(prev_index + ((int)n_samples_TAG_BIT/2 * (mask_length))) * mask_data[mask_length - 1]);

      for(int i_mask = 1; i_mask < mask_length; i_mask++){
        int i_sam = (int)n_samples_TAG_BIT/2 * i_mask;
        result += (ys->in(prev_index + i_sam) * (mask_data[i_mask-1] - mask_data[i_mask]));
      }

      result *= mask_level;
      result += prev_result;

      return result;           
    }


  } //end of rfid
} //end of gr
