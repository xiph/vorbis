#include <fuzzer/FuzzedDataProvider.h>
#include <vorbis/vorbisenc.h>

static const int sample_rates[] = {8000,  16000, 22050, 32000,
                                   44100, 48000, 96000, 192000};
static const int buffer_size = 1024;

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
  FuzzedDataProvider fdp(data, size);

  // Define encoding parameters
  const int channels = fdp.ConsumeIntegralInRange(1, 8);
  const int sample_rate = fdp.PickValueInArray(sample_rates);
  const float base_quality = fdp.ConsumeFloatingPointInRange(0.0, 1.0);

  // Temporarily skip configurations with 6 channels due to a known memory leak.
  // https://gitlab.xiph.org/xiph/vorbis/-/issues/2357
  if (channels == 6) {
    return 0;
  }

  // Declare vorbis/ogg structures
  ogg_stream_state os;
  ogg_page og;
  ogg_packet op;

  vorbis_info vi;
  vorbis_comment vc;
  vorbis_dsp_state vd;
  vorbis_block vb;

  // Initialize vorbis/ogg structures
  vorbis_info_init(&vi);
  if (vorbis_encode_init_vbr(&vi, channels, sample_rate, base_quality)) {
    return 0;
  }

  vorbis_comment_init(&vc);
  vorbis_comment_add_tag(&vc, "ENCODER", "encode_fuzzer.cc");

  vorbis_analysis_init(&vd, &vi);
  vorbis_block_init(&vd, &vb);

  ogg_stream_init(&os, 12345678);

  // Output headers
  {
    ogg_packet header;
    ogg_packet header_comm;
    ogg_packet header_code;

    vorbis_analysis_headerout(&vd, &vc, &header, &header_comm, &header_code);
    ogg_stream_packetin(&os, &header);
    ogg_stream_packetin(&os, &header_comm);
    ogg_stream_packetin(&os, &header_code);

    while (ogg_stream_flush(&os, &og)) {
    }
  }

  // Do the data encoding
  int eos = 0;
  while (!eos) {
    const int chunk = (fdp.remaining_bytes() > buffer_size * channels)
                          ? buffer_size * channels
                          : static_cast<int>(fdp.remaining_bytes());

    if (chunk == 0) {
      vorbis_analysis_wrote(&vd, 0);
    } else {
      float **buffer = vorbis_analysis_buffer(&vd, buffer_size);

      const int frames = chunk / channels;
      for (int i = 0; i < frames; i++) {
        for (int j = 0; j < channels; j++) {
          buffer[j][i] = fdp.ConsumeFloatingPointInRange(-1.0, 1.0);
        }
      }

      vorbis_analysis_wrote(&vd, frames);
    }

    while (vorbis_analysis_blockout(&vd, &vb) == 1) {
      vorbis_analysis(&vb, NULL);
      vorbis_bitrate_addblock(&vb);

      while (vorbis_bitrate_flushpacket(&vd, &op)) {
        ogg_stream_packetin(&os, &op);

        while (!eos) {
          int result = ogg_stream_pageout(&os, &og);
          if (result == 0) {
            break;
          }
          if (ogg_page_eos(&og)) {
            eos = 1;
          }
        }
      }
    }
  }

  // Clean up
  ogg_stream_clear(&os);
  vorbis_block_clear(&vb);
  vorbis_dsp_clear(&vd);
  vorbis_comment_clear(&vc);
  vorbis_info_clear(&vi);
  return 0;
}
