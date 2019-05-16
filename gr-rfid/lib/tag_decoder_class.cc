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
      _norm_in.clear();

      _corr = 0;
    }

    tag_decoder_impl::sample_information::sample_information(gr_complex* __in, int __total_size)
    // mode: 0:RN16, 1:EPC
    {
      _in = __in;
      _total_size = __total_size;
      _norm_in.clear();
      for(int i=0 ; i<_total_size ; i++)
        _norm_in.push_back(std::sqrt(std::norm(_in[i])));

      _corr = 0;
    }

    tag_decoder_impl::sample_information::~sample_information(){}

    void tag_decoder_impl::sample_information::set_corr(float __corr)
    {
      _corr = __corr;
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
      return _norm_in[index];
    }

    float tag_decoder_impl::sample_information::corr(void)
    {
      return _corr;
    }
  }
}
