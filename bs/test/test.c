#include <stdio.h>
#include "../src/block_store.c"
// including the .c lets's us see the inner working and test things easier
// than if we were using the public interface
// (you can only see inside the struct if you do it this way)

int main() {
	puts("Place your test code here!");

/*
	// assumes generated drive at
	const char *const file = "test.bs";

	block_store_t *bs = block_store_import(file);
	if (bs) {
		size_t our_block = block_store_allocate(bs);
		if (our_block) {
			size_t write_res = block_store_write(bs,our_block, file, 8, 0);
			if (write_res){
				size_t save_res = block_store_export(bs,file);
				if (save_res){
					puts(block_store_strerror(block_store_errno));
					puts("WOO HOO!");
				} else {
					puts(block_store_strerror(block_store_errno));
					puts("AHHH DID NOT EXPORT >:C");
				}
			} else {
				puts(block_store_strerror(block_store_errno));
				puts("AHH DID NOT WRITE");
			}

		} else {
			puts(block_store_strerror(block_store_errno));
			puts("AHHH NO BLOCK");
		}
	} else {
		puts(block_store_strerror(block_store_errno));
		puts("AHHHH DID NOT INIT");
	}
*/


}