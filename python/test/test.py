import lzcomplexity as lz
from lzcomplexity import sequence
import copy
# from LempelZiv import LZ_FLAGS, LempelZiv

# assert(lz.__version__ == "0.7.0")

# Test lz structures
shuffle = lz.LZ_ExcessInfo(10, 0.24, 0.002)
args = lz.LZ_Args(_chunks=5, _max_context=0, _block_size=17)

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
print(seq)

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

print(seq2[4])

res = lz.Shuffle(seq2, 2, 5)
print(res)

# # Test lempel-ziv complexity functions
args = lz.LZ_Args(_chunks=2, _max_context=0, _block_size=2)
print("Text: 01001010101101010101110101010101010000100101011")
b = sequence("01001010101101010101110101010101010000100101011")
b_copy = sequence("01001010101101010101110101010101010000100101011")
c = sequence("01001010101101010101110101110001010000100101011")
cpx = lz.LempelZivFactorization("01001010101101010101110101010101010000100101011", args)
print("factorization: ", cpx)

cpx = lz.LempelZivFactors(sequence("01001010101101010101110101010101010000100101011"), args)
print("factorization: ", cpx.factorization)
print("factors: ", cpx.lzf)

entropy = lz.EntropyDensity(b, args)
print("entropy: ", entropy)

lz_effective = lz.LZEffectiveComplexity(b, args)
print("lz effective complexity: ", lz_effective)

shuffle = lz.ShuffleEntropyDeficit(b, args)
print("shuffle entropy deficit: ", shuffle)

distance_same = lz.InformationDistance(b, b_copy, args)
print("information distance (same sequence): ", distance_same)
distance_diff = lz.InformationDistance(b, c, args)
print("information distance (diff sequence): ", distance_diff)