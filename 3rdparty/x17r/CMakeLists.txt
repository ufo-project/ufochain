add_definitions()

set(X17R_SRC
    aes_helper.c 
    gost_streebog.c 
    mod_blakecoin.c 
    sph_blake.c 
    sph_bmw.c 
    sph_cubehash.c 
    sph_echo.c 
    sph_fugue.c 
    sph_groestl.c 
    sph_hamsi_helper.c 
    sph_hamsi.c 
    sph_haval.c 
    sph_hefty1.c 
    sph_jh.c 
    sph_keccak.c 
    sph_luffa.c 
    sph_ripemd.c 
    sph_sha2.c 
    sph_sha2big.c 
    sph_shabal.c 
    sph_shavite.c 
    sph_simd.c 
    sph_skein.c 
    sph_whirlpool.c 
    x17r.c 
)

add_library(x17r STATIC ${X17R_SRC})
target_link_libraries(x17r)

