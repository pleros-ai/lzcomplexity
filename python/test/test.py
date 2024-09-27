import time
import lzcomplexity as lz
from lzcomplexity import sequence
import copy
import numpy as np
# from LempelZiv import LZ_FLAGS, LempelZiv

# assert(lz.__version__ == "0.7.0")

# Test lz structures
shuffle = lz.LZ_Shuffle(10, 0.24, 0.002)
args = lz.LZ_Args(_chunks=5, _block_size=17)

args2 = copy.deepcopy(args)
args2.block_size = 10

assert(shuffle.max_block_size == 10)
assert(shuffle.excess_value == 0.24)

assert(args.chunks == 5)
assert(args.block_size == 17)
assert(args != args2)

print(lz.__version__)
print("Suffix array implementations:")
# Test SA classes
text = "test text"
sa_res = [4, 1, 6, 2, 8, 3, 0, 5, 7]

text2 = "banana"
sa_res2 = [5, 3, 1, 0, 4, 2]

alg = lz.CaPS(text, len(text), 1, 0)
print(alg)
sa = alg.construct(text)
assert(sa.SA == sa_res)

# Test sequence
seq = sequence("Hello World!!!")
seq2 = sequence("Welcome to the jungle...")
print(seq.first())
print(seq.Min(), seq.Max())
assert(seq.first() == "H")
assert(seq.last() == "!")
assert(seq + seq2 == "Hello World!!!Welcome to the jungle...")
assert("Hello World!!!Welcome to the jungle..." == seq + seq2)
print(seq > seq2)
seq += ['1', '2', ' ', '3', '4']
print("seq: ", seq)

print(seq.reverse())
print(seq2.pi())
print(seq2.Drop(4))
print(seq2.Take(6))
print(seq2.Take(6).Drop(4))
print(seq2.rightShift(3).leftShift())
print(seq2.map(lambda x: x.upper() if x.islower() else x.lower()))
print(seq2.DetermineAlphabet())
print(seq2.alphabet_size)
print(seq2.seq)
print("length seq2: ", seq2.length())

print(seq2[4])

res = lz.Shuffle(seq2, 2, 5)
print(res)

# # Test lempel-ziv complexity functions
args = lz.LZ_Args(_chunks=2, _block_size=2)
print("Text: 01001010101101010101110101010101010000100101011")
b = sequence("01001010101101010101110101010101010000100101011")
b_copy = sequence("01001010101101010101110101010101010000100101011")
c = sequence("01001010101101010101110101110001010000100111001")
cpx = lz.lz76Factorization("01001010101101010101110101010101010000100101011")
print("factorization: ", cpx)

cpx = lz.lz76Factors("01001010101101010101110101010101010000100101011", args)
print("factorization: ", cpx.factorization)
print("factors: ", cpx.lzf)
cpx = lz.lz76Factors("01001010101101010101110101010101010000100101011", partitions=1, jobs=1)
print("factorization: ", cpx.factorization)
print("factors: ", cpx.lzf)

entropy = lz.lz76EntropyDensity("01001010101101010101110101010101010000100101011", args)
print("entropy: ", entropy)

# lz_effective = lz.LZEffectiveComplexity(b, args)
# print("lz effective complexity: ", lz_effective)

# shuffle = lz.ShuffleEntropyDeficit(b, args)
# print("shuffle entropy deficit: ", shuffle)

distance_same = lz.lz76InformationDistance(b, b_copy, args)
print("information distance (same sequence): ", distance_same)
distance_diff = lz.lz76InformationDistance(b, c, args)
print("information distance (diff sequence): ", distance_diff)

res = lz.lz76(b, args)
print("Result LZ:")
print(res.complexity)
print(res.entropy)
print(res.random_shuffle_complexity.excess_value)
print(res.factors)
print(res.extras.redundancy)

# Test with specific sequence size
print("Random sequence of length 2048")
# str = "".join([str(np.random.choice([0, 1], p=[0.5, 0.5])) for i in range(2048)])
str_example = "11111101010111111100000011000011111011111010000010001101000010011100100101011000100111111111000011111000000010111000111111100000000011101100001100000110111100111000011000100111010000000011111110100001110110101001010100000101111111111101001010001110010000111111011001010111010011111011111111001111101000001010101011101111101011111110010011000000110100011000000111111001010110000100110110010010001101100010000111010001001010000000000000001100000001101111110010011101011001100000001101101010001000000011011100011010011011110110011010001010100110110100010100111101101101110101111011101001001111111110101110001001110001000001110111010110010001011010101010101100111010000001001110010001000010000001010111110100111000010001100111110000010010110001011000111001001101110011011100011101100001011100110010100100111100001000110110100011111001010011011111011100001001010110001100010011001001101001100111111100000010110100000010110001001011100110010100010011011111011110100100111011010011110001010001100101000000111000100001001111010110100010100001001111111111000110110001101101100101000100101101000100110101001100100100000111100101110111110111000110101010100011100000111000111011010100010110011110101010110001100101011010001100010100000110001000001101110010010010101011111100100101111000011011110010001110010011000010010011101110110001000111100111000001111101000011111010010110011111011010110111010010101001001110101000000101110110011010011101110000010101111011100010001010010011001101001010010110011100100101010110100010001011110111101000011101110000101000001110001000111101001011110000011011011001000000001110000101101011001100010001111101001100001011000011110101010101011110101011101110010000100110100000111111101101101111110000001110001000011000100111100101000011001011000110001100010101001111010000001101101110110101011011101011110100000111000101001110011111011110101111111011001000000001110011100101100001000101111000001111100010100111000011111111110101011010011000111111000101100110000001100101111000001001111011111110100101110110100100101000001011011110"

args = lz.LZ_Args(_chunks=2)
cpx = lz.lz76Factors(str_example, args)
print("factors: ", cpx.factorization)
print("lzf: ", cpx.lzf)

shuffle = lz.lz76RandomShuffleComplexity(str_example, args)
print("excess_value: ", shuffle.excess_value)
print("summands: ", shuffle.summands)


a = np.random.randint(2, size=100000)
start = time.time()
a_lz = sequence(a)
end = time.time()
print("time: ", end - start)
print(a_lz.length())

d = lz.lz76EntropyDensity(a_lz)
s = lz.lz76RandomShuffleComplexity(a_lz)

print(a[:10])
d1 = lz.lz76EntropyDensity(a)
s1 = lz.lz76RandomShuffleComplexity(a)
print(a[:10])
d2 = lz.lz76EntropyDensity(str_example)
s2 = lz.lz76RandomShuffleComplexity(str_example)

print(d, a_lz.length())
print(a_lz.seq[:10])
# 1.0180048646782323
print(s)
# shuffle entropy deficit: 0.028410, block size: 22
print(d1)
print(s1)
print(d2)
print(s2)