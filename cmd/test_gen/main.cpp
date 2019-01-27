#include "crypto.h"
#include "test_vectors.h"
#include "tree_math.h"
#include <iostream>

void
generate_tree_math(TestVectors& vectors)
{
  for (int n = 1; n <= TreeMathTestVectors::tree_size; ++n) {
    auto val = mls::tree_math::root(n);
    vectors.tree_math.root.push_back(val);
  }

  auto n = TreeMathTestVectors::tree_size;
  for (int x = 0; x < TreeMathTestVectors::tree_size; ++x) {
    auto left = mls::tree_math::left(x);
    vectors.tree_math.left.push_back(left);

    auto right = mls::tree_math::right(x, n);
    vectors.tree_math.right.push_back(right);

    auto parent = mls::tree_math::parent(x, n);
    vectors.tree_math.parent.push_back(parent);

    auto sibling = mls::tree_math::sibling(x, n);
    vectors.tree_math.sibling.push_back(sibling);
  }
}

void
generate_crypto(TestVectors& vectors)
{
  std::vector<CipherSuite> suites{
    CipherSuite::P256_SHA256_AES128GCM,
    CipherSuite::X25519_SHA256_AES128GCM,
    CipherSuite::P521_SHA512_AES256GCM,
    CipherSuite::X448_SHA512_AES256GCM,
  };

  std::vector<CryptoTestVectors::TestCase*> cases{
    &vectors.crypto.case_p256,
    &vectors.crypto.case_x25519,
    &vectors.crypto.case_p521,
    &vectors.crypto.case_x448,
  };

  std::string derive_secret_label_string = "test";
  bytes derive_secret_label_bytes(derive_secret_label_string.begin(),
                                  derive_secret_label_string.end());

  vectors.crypto.hkdf_extract_salt = { 0, 1, 2, 3 };
  vectors.crypto.hkdf_extract_ikm = { 4, 5, 6, 7 };

  vectors.crypto.derive_secret_secret = bytes(32, 0xA0);
  vectors.crypto.derive_secret_label = derive_secret_label_bytes;
  vectors.crypto.derive_secret_length = 32;

  vectors.crypto.derive_key_pair_seed = { 0, 1, 2, 3 };

  vectors.crypto.ecies_seed = bytes(32, 0xB0);
  vectors.crypto.ecies_plaintext = bytes(128, 0xB1);

  // Construct a test case for each suite
  for (int i = 0; i < suites.size(); ++i) {
    auto suite = suites[i];
    auto test_case = cases[i];

    // HKDF-Extract
    test_case->hkdf_extract_out = mls::hkdf_extract(
      suite, vectors.crypto.hkdf_extract_salt, vectors.crypto.hkdf_extract_ikm);

    // Derive-Secret
    // TODO(rlb@ipv.sx): Populate some actual state
    test_case->derive_secret_state = GroupState{ suite };
    test_case->derive_secret_out =
      mls::derive_secret(suite,
                         vectors.crypto.derive_secret_secret,
                         derive_secret_label_string,
                         test_case->derive_secret_state,
                         vectors.crypto.derive_secret_length);

    // Derive-Key-Pair
    auto priv =
      DHPrivateKey::derive(suite, vectors.crypto.derive_key_pair_seed);
    auto pub = priv.public_key();
    test_case->derive_key_pair_pub = pub;

    // ECIES
    test_case->ecies_out =
      pub.encrypt(vectors.crypto.ecies_seed, vectors.crypto.ecies_plaintext);
  }
}

void
generate_messages(TestVectors& vectors)
{
  std::vector<CipherSuite> suites{
    CipherSuite::P256_SHA256_AES128GCM,
    CipherSuite::X25519_SHA256_AES128GCM,
    CipherSuite::P521_SHA512_AES256GCM,
    CipherSuite::X448_SHA512_AES256GCM,
  };

  std::vector<SignatureScheme> schemes{
    SignatureScheme::P256_SHA256,
    SignatureScheme::Ed25519,
    SignatureScheme::P521_SHA512,
    SignatureScheme::Ed448,
  };

  std::vector<MessagesTestVectors::TestCase*> cases{
    &vectors.messages.case_p256_p256,
    &vectors.messages.case_x25519_ed25519,
    &vectors.messages.case_p521_p521,
    &vectors.messages.case_x448_ed448,
  };

  // Construct a UIK with all the ciphersuites
  auto& uik_all = vectors.messages.user_init_key_all;
  for (const auto& suite : suites) {
    auto priv = DHPrivateKey::generate(suite);
    uik_all.add_init_key(priv.public_key());
  }

  auto user_id = random_bytes(4);
  auto identity_priv = SignaturePrivateKey::generate(SignatureScheme::Ed25519);
  auto credential_all = Credential::basic(user_id, identity_priv);
  uik_all.sign(identity_priv, credential_all);

  // Construct a test case for each suite
  for (int i = 0; i < suites.size(); ++i) {
    auto suite = suites[i];
    auto scheme = schemes[i];
    auto test_case = cases[i];

    // Miscellaneous data items we need to construct messages
    auto dh_key = DHPrivateKey::generate(suite).public_key();
    auto sig_priv = SignaturePrivateKey::generate(scheme);
    auto sig_key = sig_priv.public_key();

    auto group_id = random_bytes(4);
    auto uik_id = random_bytes(4);

    auto epoch = epoch_t(0x42);
    auto signer_index = uint32_t(0);
    auto removed = uint32_t(1);

    auto random = random_bytes(Digest(suite).output_size());
    auto ratchet_tree =
      RatchetTree{ suite, { random, random, random, random } };
    ratchet_tree.blank_path(2);
    auto direct_path = ratchet_tree.encrypt(0, random);

    auto cred = Credential::basic(user_id, sig_key);
    auto roster = Roster{};
    roster.add(cred);

    // Construct UIK
    test_case->user_init_key.user_init_key_id = uik_id;
    test_case->user_init_key.add_init_key(dh_key);
    test_case->user_init_key.sign(sig_priv, cred);

    // Construct WelcomeInfo and Welcome
    test_case->welcome_info = {
      group_id, epoch, roster, ratchet_tree, random, random,
    };
    test_case->welcome = { uik_id, dh_key, test_case->welcome_info };

    // Construct Handshake messages
    auto add = Add{ test_case->user_init_key };
    auto update = Update{ direct_path };
    auto remove = Remove{ removed, direct_path };
    test_case->add = { epoch, add, signer_index, random, random };
    test_case->update = { epoch, update, signer_index, random, random };
    test_case->remove = { epoch, remove, signer_index, random, random };
  }
}

int
main()
{
  TestVectors vectors;

  // Generate and write test vectors
  generate_tree_math(vectors);
  generate_crypto(vectors);
  generate_messages(vectors);
  vectors.dump();

  // Verify that the test vectors load
  try {
    TestVectors::get();
  } catch (...) {
    std::cerr << "Error: Generated test vectors failed to load" << std::endl;
  }
  return 0;
}
