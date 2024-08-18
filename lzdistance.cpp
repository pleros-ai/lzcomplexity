#include <bzlib.h>
#include <lz/lz.h>
#include <zlib.h>

#include <fstream>
#include <iomanip>
#include <iostream>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>

#include "main/main.h"

std::string compress_zlib(const std::string& str, int compressionlevel = Z_BEST_COMPRESSION) {
   z_stream zs;  // z_stream is zlib's control structure
   memset(&zs, 0, sizeof(zs));

   if (deflateInit2(&zs, compressionlevel, Z_DEFLATED, 8, 1, Z_DEFAULT_STRATEGY) != Z_OK)
      throw(std::runtime_error("deflateInit failed while compressing."));

   zs.next_in  = (Bytef*)str.data();
   zs.avail_in = str.size();  // set the z_stream's input

   int         ret;
   char        outbuffer[32768];
   std::string outstring;

   // retrieve the compressed bytes blockwise
   do {
      zs.next_out  = reinterpret_cast<Bytef*>(outbuffer);
      zs.avail_out = sizeof(outbuffer);

      ret = deflate(&zs, Z_FINISH);

      // ret = compress2(reinterpret_cast<Bytef*>(outbuffer), &l, (Bytef*)str.data(), str.length(), Z_BEST_COMPRESSION);

      if (outstring.size() < zs.total_out) {
         // append the block to the output string
         outstring.append(outbuffer, zs.total_out - outstring.size());
      }
      std::cout << outstring << std::endl;
      break;
   } while (ret == Z_OK);

   deflateEnd(&zs);

   if (ret != Z_STREAM_END) {  // an error occurred that was not EOF
      std::ostringstream oss;
      oss << "Exception during zlib compression: (" << ret << ") " << zs.msg;
      throw(std::runtime_error(oss.str()));
   }

   return outstring;
}

auto compress_gzip(const std::string& str, int compressionlevel = Z_BEST_COMPRESSION) {
   z_stream zs;  // z_stream is zlib's control structure
   memset(&zs, 0, sizeof(zs));

   if (deflateInit2(&zs, compressionlevel, Z_DEFLATED, 8, 1, Z_FILTERED) != Z_OK)
      throw(std::runtime_error("deflateInit failed while compressing."));

   zs.next_in  = (Bytef*)str.data();
   zs.avail_in = str.size();  // set the z_stream's input

   int         ret;
   char        outbuffer[32768];
   std::string outstring;

   // retrieve the compressed bytes blockwise
   do {
      zs.next_out  = reinterpret_cast<Bytef*>(outbuffer);
      zs.avail_out = sizeof(outbuffer);

      ret = deflate(&zs, Z_FINISH);

      // ret = compress2(reinterpret_cast<Bytef*>(outbuffer), &l, (Bytef*)str.data(), str.length(), Z_BEST_COMPRESSION);

      if (outstring.size() < zs.total_out) {
         // append the block to the output string
         outstring.append(outbuffer, zs.total_out - outstring.size());
      }
      std::cout << outstring << std::endl;
      break;
   } while (ret == Z_OK);

   deflateEnd(&zs);

   if (ret != Z_STREAM_END) {  // an error occurred that was not EOF
      std::ostringstream oss;
      oss << "Exception during zlib compression: (" << ret << ") " << zs.msg;
      throw(std::runtime_error(oss.str()));
   }

   return outstring;
}

auto compress_bzip(const std::string& str) {
   bz_stream bz;
   memset(&bz, 0, sizeof(bz));

   // 9 best compression
   if (BZ2_bzCompressInit(&bz, 9, 0, 30) != BZ_OK)
      throw(std::runtime_error("deflateInit failed while compressing."));

   bz.next_in  = (char*)str.data();
   bz.avail_in = str.size();  // set the z_stream's input

   int         ret;
   char        outbuffer[32768];
   std::string outstring;

   // retrieve the compressed bytes blockwise
   do {
      bz.next_out  = reinterpret_cast<char*>(outbuffer);
      bz.avail_out = sizeof(outbuffer);

      ret = BZ2_bzCompress(&bz, BZ_FINISH);

      std::cout << bz.avail_in << " " << bz.total_out_hi32 << " " << bz.total_out_lo32 << std::endl;

      if (outstring.size() < bz.total_out_lo32) {
         // append the block to the output string
         outstring.append(outbuffer, bz.total_out_lo32 - outstring.size());
      }
      std::cout << outstring << std::endl;
      break;
   } while (ret == BZ_OK);

   BZ2_bzCompressEnd(&bz);

   if (ret != BZ_STREAM_END) {  // an error occurred that was not EOF
      std::ostringstream oss;
      oss << "Exception during zlib compression: (" << ret << ") " << bz.state;
      throw(std::runtime_error(oss.str()));
   }

   return outstring;
}

/**
 * @details even two state process
 *
 * T0 = 1-p 0      T1 = 0 p
 *      p   0           0 1-p
 *
 * @param p probability of the transition
 * @param size size of the sequence. Defaults to 256.
 */
std::string even_process(double p, unsigned size = 256) {
   short       state = 0;
   unsigned    idx   = 0;
   std::string seq;

   std::random_device rd_seed;
   std::mt19937       random_engine(rd_seed());

   // std::normal_distribution dist;
   std::uniform_real_distribution<> dist(0, 1);

   while (idx++ < size) {
      double r = dist(random_engine);

      if (state == 0) {
         if (r <= p) {
            state = 1;
            seq += "0";
         } else {
            seq += "1";
         }
      } else {
         state = 0;
         seq += "0";
      }
   }

   return seq;
}

/**
 * @details two symmetric state process
 *
 * T0 = 1-p 0      T1 = 0 p
 *      p   0           0 1-p
 *
 * @param p probability of the transition
 * @param size size of the sequence. Defaults to 256.
 */
std::string symmetric_process(double p, unsigned size = 256) {
   short       state = 0;
   unsigned    idx   = 0;
   std::string seq;

   std::random_device rd_seed;
   std::mt19937       random_engine(rd_seed());

   // std::normal_distribution dist;
   std::uniform_real_distribution<> dist(0, 1);

   while (idx++ < size) {
      double r = dist(random_engine);

      if (state == 0) {
         if (r <= p) {
            state = 1;
            seq += "0";
         } else {
            seq += "1";
         }
      } else {
         if (r <= p) {
            state = 0;
            seq += "1";
         } else {
            seq += "0";
         }
      }
   }

   return seq;
}

template<typename Fun>
void generate_file(std::vector<double> sizes, Fun&& gen, std::string process) {
   for (auto sz: sizes) {
      for (auto i = 0.0; i <= 1.01; i += 0.05) {
         auto res = gen(i, sz);

         std::string s = std::to_string((int)sz);

         std::stringstream stream;
         stream << std::fixed << std::setprecision(2) << i;
         std::string pr = stream.str();

         std::ofstream out(process + s + "_prob" + pr);

         if (out.is_open() && out.good()) {
            out << res;
         }

         out.close();
      }
   }
}

void generate_h_result_file(std::vector<double> sizes, std::string process) {
   std::vector<double> res;
   lz::utils::EnabledMT(1);
   for (auto sz: sizes) {
      std::string s = std::to_string((int)sz);
      for (auto i = 0.0; i < 1.01; i += 0.05) {
         std::stringstream stream;
         stream << std::fixed << std::setprecision(2) << std::fabs(1 - i);
         std::string pr = stream.str();

         std::ifstream in(process + s + "_prob" + pr);
         std::string   seq;

         if (in.is_open() && in.good()) {
            in >> seq;

            lz::utils::LZ_Args args;
            args.chunks = 2;
            auto h      = lz::lz76EntropyDensity(seq, args);
            std::cout << h << " - " << pr << " - " << i << std::endl;
            res.push_back(h);
         }

         in.close();
      }

      std::ofstream out(process + s + "_h");
      if (out.is_open() && out.good()) {
         for (auto r: res) {
            out << r << " ";
         }
      }

      out.close();
   }
   lz::utils::DisabledMT();
}

void generate_E_result_file(std::vector<double> sizes, std::string process) {
   std::vector<double> res;
   lz::utils::EnabledMT(1);
   for (auto sz: sizes) {
      std::string s = std::to_string((int)sz);
      for (auto i = 0.0; i < 1.01; i += 0.05) {
         std::stringstream stream;
         stream << std::fixed << std::setprecision(2) << std::fabs(1 - i);
         std::string pr = stream.str();

         std::ifstream in(process + s + "_prob" + pr);
         std::string   seq;

         if (in.is_open() && in.good()) {
            in >> seq;

            lz::utils::LZ_Args args;
            args.chunks = 2;
            auto E      = lz::lz76RandomShuffleComplexity(seq, args);
            std::cout << E.excess_value << " - " << pr << " - " << i << std::endl;
            res.push_back(E.excess_value);
         }

         in.close();
      }

      std::ofstream out(process + s + "_E");
      if (out.is_open() && out.good()) {
         for (auto r: res) {
            out << r << " ";
         }
      }

      out.close();
   }
   lz::utils::DisabledMT();
}

int main() {

   std::cout << "LZ distance..." << std::endl;

   std::string z = "1001010101001011101010100010010101101";

   auto lz = lz::lz76EntropyDensity(z);

   std::cout << lz << std::endl;

   auto z_res = compress_zlib(z);
   std::cout << z.size() << " -- " << z_res.size() / (double)z.size() << std::endl;

   auto gz_res = compress_gzip(z);
   std::cout << z.size() << " -- " << gz_res.size() / (double)z.size() << std::endl;

   auto bzip_res = compress_bzip(z);
   std::cout << z.size() << " -- " << bzip_res.size() / (double)z.size() << std::endl;

   std::vector<double> sizes = {256, 1e3, 1e4, 1e5, 1e6, 1e7, 1e8, 1e9};

   // generate_file({256}, symmetric_process, "symmetric");

   generate_E_result_file({256}, "even");
   // generate_h_result_file({256}, "symmetric");

   return 0;
}