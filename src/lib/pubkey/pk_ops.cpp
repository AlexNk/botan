/*
* PK Operation Types
* (C) 2010,2015 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#include <botan/internal/pk_ops_impl.h>
#include <botan/eme.h>
#include <botan/kdf.h>
#include <botan/emsa.h>
#include <botan/internal/bit_ops.h>

namespace Botan {

PK_Ops::Encryption_with_EME::Encryption_with_EME(const std::string& eme)
   {
   m_eme.reset(get_eme(eme));
   if(!m_eme.get())
      throw Algorithm_Not_Found(eme);
   }

PK_Ops::Encryption_with_EME::~Encryption_with_EME() {}

size_t PK_Ops::Encryption_with_EME::max_input_bits() const
   {
   return m_eme->maximum_input_size(max_raw_input_bits());
   }

secure_vector<byte> PK_Ops::Encryption_with_EME::encrypt(const byte msg[], size_t msg_len,
                                                         RandomNumberGenerator& rng)
   {
   const size_t max_raw = max_raw_input_bits();

   const std::vector<byte> encoded = unlock(m_eme->encode(msg, msg_len, max_raw, rng));

   if(8*(encoded.size() - 1) + high_bit(encoded[0]) > max_raw)
      throw std::runtime_error("Input is too large to encrypt with this key");

   return raw_encrypt(encoded.data(), encoded.size(), rng);
   }

PK_Ops::Decryption_with_EME::Decryption_with_EME(const std::string& eme)
   {
   m_eme.reset(get_eme(eme));
   if(!m_eme.get())
      throw Algorithm_Not_Found(eme);
   }

PK_Ops::Decryption_with_EME::~Decryption_with_EME() {}

size_t PK_Ops::Decryption_with_EME::max_input_bits() const
   {
   return m_eme->maximum_input_size(max_raw_input_bits());
   }

secure_vector<byte> PK_Ops::Decryption_with_EME::decrypt(const byte msg[], size_t length)
   {
   return m_eme->decode(raw_decrypt(msg, length), max_raw_input_bits());
   }

PK_Ops::Key_Agreement_with_KDF::Key_Agreement_with_KDF(const std::string& kdf)
   {
   if(kdf != "Raw")
      m_kdf.reset(get_kdf(kdf));
   }

PK_Ops::Key_Agreement_with_KDF::~Key_Agreement_with_KDF() {}

secure_vector<byte> PK_Ops::Key_Agreement_with_KDF::agree(size_t key_len,
                                                          const byte w[], size_t w_len,
                                                          const byte salt[], size_t salt_len)
   {
   secure_vector<byte> z = raw_agree(w, w_len);
   if(m_kdf)
      return m_kdf->derive_key(key_len, z, salt, salt_len);
   return z;
  }

PK_Ops::Signature_with_EMSA::Signature_with_EMSA(const std::string& emsa)
   {
   m_emsa.reset(get_emsa(emsa));
   if(!m_emsa)
      throw Algorithm_Not_Found(emsa);
   }

PK_Ops::Signature_with_EMSA::~Signature_with_EMSA() {}

void PK_Ops::Signature_with_EMSA::update(const byte msg[], size_t msg_len)
   {
   m_emsa->update(msg, msg_len);
   }

secure_vector<byte> PK_Ops::Signature_with_EMSA::sign(RandomNumberGenerator& rng)
   {
   const secure_vector<byte> msg = m_emsa->raw_data();
   const auto padded = m_emsa->encoding_of(msg, this->max_input_bits(), rng);
   return raw_sign(padded.data(), padded.size(), rng);
   }

PK_Ops::Verification_with_EMSA::Verification_with_EMSA(const std::string& emsa)
   {
   m_emsa.reset(get_emsa(emsa));
   if(!m_emsa)
      throw Algorithm_Not_Found(emsa);
   }

PK_Ops::Verification_with_EMSA::~Verification_with_EMSA() {}

void PK_Ops::Verification_with_EMSA::update(const byte msg[], size_t msg_len)
   {
   m_emsa->update(msg, msg_len);
   }

bool PK_Ops::Verification_with_EMSA::is_valid_signature(const byte sig[], size_t sig_len)
   {
   const secure_vector<byte> msg = m_emsa->raw_data();

   if(with_recovery())
      {
      secure_vector<byte> output_of_key = verify_mr(sig, sig_len);
      return m_emsa->verify(output_of_key, msg, max_input_bits());
      }
   else
      {
      Null_RNG rng;
      secure_vector<byte> encoded = m_emsa->encoding_of(msg, max_input_bits(), rng);
      return verify(encoded.data(), encoded.size(), sig, sig_len);
      }
   }

}
