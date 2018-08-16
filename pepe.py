
def bitprim_requires(s):
    s0 = s[0] % ("bitprim", "stable")
    print(s0)

bitprim_requires(["bitprim-core/0.X@%s/%s", "bitprim-core/0.X@%s/%s"])