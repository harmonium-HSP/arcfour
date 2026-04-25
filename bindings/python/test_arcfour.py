import unittest
import os
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from arcfour import ArcFour, ArcFourAEAD

class TestArcFour(unittest.TestCase):
    def test_encrypt_decrypt_consistency(self):
        key = b"test_key_12345"
        plaintext = b"Hello, RC4 encryption!"
        
        with ArcFour(key) as cipher:
            ciphertext = cipher.encrypt(plaintext)
        
        with ArcFour(key) as cipher:
            decrypted = cipher.decrypt(ciphertext)
        
        self.assertEqual(plaintext, decrypted)
    
    def test_same_key_same_output(self):
        key = b"consistent_key"
        plaintext = b"test data"
        
        cipher1 = ArcFour(key)
        cipher2 = ArcFour(key)
        
        result1 = cipher1.encrypt(plaintext)
        result2 = cipher2.encrypt(plaintext)
        
        self.assertEqual(result1, result2)
        
        cipher1.close()
        cipher2.close()
    
    def test_different_keys_different_output(self):
        key1 = b"key_one"
        key2 = b"key_two"
        plaintext = b"same plaintext"
        
        cipher1 = ArcFour(key1)
        cipher2 = ArcFour(key2)
        
        result1 = cipher1.encrypt(plaintext)
        result2 = cipher2.encrypt(plaintext)
        
        self.assertNotEqual(result1, result2)
        
        cipher1.close()
        cipher2.close()

class TestArcFourAEAD(unittest.TestCase):
    def test_encrypt_decrypt_consistency(self):
        key = ArcFourAEAD.generate_key()
        aead = ArcFourAEAD(key)
        plaintext = b"Secret message for AEAD"
        
        nonce, ciphertext, tag = aead.encrypt(plaintext)
        
        decrypted = aead.decrypt(nonce, ciphertext, tag)
        
        self.assertEqual(plaintext, decrypted)
    
    def test_with_aad(self):
        key = ArcFourAEAD.generate_key()
        aead = ArcFourAEAD(key)
        plaintext = b"Message with metadata"
        aad = b"Some metadata"
        
        nonce, ciphertext, tag = aead.encrypt(plaintext, aad)
        
        decrypted = aead.decrypt(nonce, ciphertext, tag, aad)
        
        self.assertEqual(plaintext, decrypted)
    
    def test_tampered_tag_fails(self):
        key = ArcFourAEAD.generate_key()
        aead = ArcFourAEAD(key)
        plaintext = b"Test tamper detection"
        
        nonce, ciphertext, tag = aead.encrypt(plaintext)
        
        tampered_tag = bytearray(tag)
        tampered_tag[0] ^= 0xFF
        
        with self.assertRaises(ValueError):
            aead.decrypt(nonce, ciphertext, bytes(tampered_tag))
    
    def test_tampered_ciphertext_fails(self):
        key = ArcFourAEAD.generate_key()
        aead = ArcFourAEAD(key)
        plaintext = b"Test ciphertext integrity"
        
        nonce, ciphertext, tag = aead.encrypt(plaintext)
        
        tampered_cipher = bytearray(ciphertext)
        tampered_cipher[0] ^= 0xFF
        
        with self.assertRaises(ValueError):
            aead.decrypt(nonce, bytes(tampered_cipher), tag)
    
    def test_wrong_aad_fails(self):
        key = ArcFourAEAD.generate_key()
        aead = ArcFourAEAD(key)
        plaintext = b"Test AAD verification"
        aad = b"Correct AAD"
        
        nonce, ciphertext, tag = aead.encrypt(plaintext, aad)
        
        wrong_aad = b"Wrong AAD"
        
        with self.assertRaises(ValueError):
            aead.decrypt(nonce, ciphertext, tag, wrong_aad)

if __name__ == "__main__":
    import sys
    unittest.main()
