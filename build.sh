nasm -felf64 man_asm.asm -o man_asm.o
gcc -c avl_tree.c -O3 -fno-strict-aliasing -std=c11 -march=native -Wpedantic -flto 
gcc -c conds.c -O3 -fno-strict-aliasing -std=c11 -march=native -Wpedantic -flto
gcc -c man.c -O3 -fno-strict-aliasing -std=c11 -march=native -Wpedantic -flto
gcc -c queue.c -O3 -fno-strict-aliasing -std=c11 -march=native -Wpedantic -flto
gcc -c rings.c -O3 -fno-strict-aliasing -std=c11 -march=native -Wpedantic -flto
gcc -c thread.c -O3 -fno-strict-aliasing -std=c11 -march=native -Wpedantic -flto
gcc -c time_ns.c -O3 -fno-strict-aliasing -std=c11 -march=native -Wpedantic -flto
gcc -c timer.c -O3 -fno-strict-aliasing -std=c11 -march=native -Wpedantic -flto
ar rcs libgreenman.a *.o
rm *.o