/* -*- c++ -*- */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "tag_decoder_impl.h"

namespace gr
{
  namespace rfid
  {
    tag_decoder_impl::sample_information::sample_information()
    {
      _in = NULL;
      _total_size = 0;
      _corr = 0;
      _complex_corr = std::complex<float>(0.0,0.0);

    }

    tag_decoder_impl::sample_information::sample_information(gr_complex* __in, int __total_size)
    // mode: 0:RN16, 1:EPC
    {
      this->_in = __in;
      this->_total_size = __total_size;
      _corr = 0;
      _complex_corr = std::complex<float>(0.0,0.0);
    }

    tag_decoder_impl::sample_information::~sample_information(){}

    void tag_decoder_impl::sample_information::set_corr(float __corr)
    {
      _corr = __corr;
    }

    void tag_decoder_impl::sample_information::set_complex_corr(gr_complex __complex_corr)
    {
      _complex_corr = __complex_corr;
    }

    gr_complex tag_decoder_impl::sample_information::in(int index)
    {
      return _in[index];
    }

    int tag_decoder_impl::sample_information::total_size(void)
    {
      return _total_size;
    }

    float tag_decoder_impl::sample_information::norm_in(int index)
    {
      return std::abs(_in[index]);
    }

    float tag_decoder_impl::sample_information::corr(void)
    {
      return _corr;
    }

    gr_complex tag_decoder_impl::sample_information::complex_corr(void)
    {
      return _complex_corr;
    }
  }
}
