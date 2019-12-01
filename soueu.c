// Autor: Rodrigo Schio Wengenroth Silva.
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/random.h>

typedef __uint128_t uint128;
typedef __int128_t int128;

typedef union pu128 {
	uint128 raw;
	uint8_t reversedBytes[16];	
} pu128;

uint128 bitArr[128] = {0};

// init_bitArr fill the bitArr[] where i is the lenght of bits necessary to 
// represent the bitArr[i]. Consider 0 needs 0 bits to be represented.
void init_bitArr() {
	bitArr[0] = 0;
	int i = 0;
	for (i = 0; i < 127; i++) {
		uint128 n = (uint128)((uint128)1 << i);
		bitArr[i+1] = n;
	}
}

// bitlen returns the maximum bit lenght needed to represent n.
int bitlen(uint128 n) {
	// Lazy init.
	if (bitArr[1] == 0) init_bitArr();
	int i = 0;
	for (i = 0; i < 128; i++) {
		if (n <= bitArr[i]) {
			return i;
		}
	}
	return i;
}

// randIntN returns a uniform random value in [2, max).
// Adapted from https://golang.org/src/crypto/rand/util.go.
uint64_t randIntN(uint64_t max) {
	if (max <= 2) return 0;
	uint64_t n = (uint64_t)(max - (uint64_t)1);
	int blen = bitlen(n);
	if (blen == 0) return 0;
	// k is the maximum byte lenght needed to represent a value < max.
	int k = (blen + 7) / 8;
	// b is the number of bits in the most significant byte of max-1.
	uint32_t b = (uint32_t)(blen % 8);
	if (b == 0) b = 8;
	
	pu128 p = {0};
	for (;;) {
		getrandom(p.reversedBytes, k, GRND_NONBLOCK);
		// Clear bits in the first byte to increase the probability
		// that the candidate is < max.
		p.reversedBytes[k-1] &= (uint8_t)((int)(1<<b) - 1);
		if (p.raw < max && p.raw >= 2) {
			return p.raw;
		}
	}
}

// multMod returns (a*b) % n.
uint64_t multMod(uint64_t a, uint64_t b, uint64_t n) {
	if (n <= 2) return 0;	
	// Fast path.
	if (a == b) {
		uint128 x = 0;
		x = a % n;
		x *= x;
		return x % n;
	}
	return (uint128)((uint128)(a % n) * (uint128)(b % n)) % (uint128)n;
}

// calcN calculate a n that is p * q and will be used
// to do mod of some operations.
static inline uint128 calcN(uint64_t p, uint64_t q) {
	return (uint128)((uint128)p * (uint128)q);
}

// __modInverse calculates the x as (u*x) mod v = 1 and set gcd of it
// that should be 1 if x is valid.
static uint64_t __modInverse(uint64_t u, uint64_t v, uint64_t *gcd) {
	uint64_t x = 1;
	uint64_t y = 0;
	uint64_t vIni = v;

	uint128 w = 0;
	uint128 t = 0;
	
	// fix the alg.
	int bitIt = 1;
	while(v != 0) {
		uint64_t q = u / v;
		// -- gcd  --
		uint64_t r = u % v;
		u = v;
		v = r;
		// ---
		w = q * y;
		t = x + w;
		x = y;
		y = t;

		bitIt = -bitIt;
	}
	*gcd = u;

	if (bitIt < 0) x = vIni - x;
	return x;
}

// modInverse calculates the x as (u*x) mod v = 1 if u and v
// has no divisor in commom (except 1) else returns 0.
uint64_t modInverse(uint64_t u, uint64_t v) {
	uint64_t gcd = 0;
	return __modInverse(u, v, &gcd) * (gcd == 1);
}

// verifynsv verify if n s and v are corrects.
int verifynsv(uint64_t n, uint64_t s, uint64_t v) {
	if (n < 3) return 0;
	if (2 > s || s >= n) return 0;
	int128 c = (int128) n;
	int128 a = s % c;
	int128 b = v % c;
	a = (a * a) % c;
	a = (a * b) % c;
	return a == 1;
}

// fFunc is the fabio function.
void fFunc() {
	uint64_t n = 0;
	uint64_t s = 0;
	uint64_t v = 0;
	uint64_t r = 0;
	uint64_t x = 0;
	int times = 0;
	char b = 0;
	char mode = 0;
	for (;;) {
		scanf(" %c", &mode);
		switch (mode) {
		case 'I':
		case 'i':
			scanf("%llu %llu %llu", &n, &s, &v);
			int ok = verifynsv(n, s, v);
			if (!ok) {
				n = 0; s = 0; v = 0;
				printf("E\n");
			} else {
				printf("C\n");
			}
			times = 0;
			break;
		case 'X':
		case 'x':
			if (n == 0) {
				printf("E\n");
				break;
			}
			r = randIntN(n);
			x = multMod(r, r, n);
			printf("C %llu\n", x);			
			break;
		case 'P':
		case 'p':
			scanf("%llu", &r);
			if (2 > r || r >= n || n == 0) {
				printf("E\n");
				break;
			}
			x = multMod(r, r, n);
			printf("C %llu\n", x);			
			times = 0;
			break;	
		case 'R':
		case 'r':
			b = 0;
			scanf(" %c", &b);
			if (n == 0 || times > 0 || '0' > b || b > '1') {
				printf("E\n");
				break;
			}
			times++;
			printf("C ");
			if (b == '0') printf("%llu\n", r);
			if (b == '1') {
				uint64_t xb = multMod(r, s, n);
				printf("%llu\n", xb); 
			}
			r = 0;
			break;
		case 'T':
		case 't':
			printf("C\n");
			return;
		default:
			printf("E\n");
		}
		mode = 0;
	}
}

// pFunc is the Patricia function.
void pFunc(){
	pu128 randSrc = {0};
	uint64_t n, v, x, xb = 0;
	uint32_t t = 0;
	uint32_t rememberT = 0;
	int block = 0;
	char b, mode = 0;
reset:
	// 7 Bytes = 56 bits > 50 bits.
	getrandom(randSrc.reversedBytes, 7, GRND_NONBLOCK);
	x = 0; xb = 0; t = rememberT; block = 0; b = 0; mode = 0;
	for (;;) {
		scanf(" %c", &mode);
		switch (mode) {
		case 'I':	
		case 'i':	
			scanf("%llu %llu %u", &n, &v, &t);
			if (n < 3 || 3 > t || t > 50) {
				printf("E\n");
				break;
			}
			rememberT = t;
			printf("C\n");
			break;
		case 'Q':
		case 'q':
			if (block) {
				printf("E\n");
				break;
			}
			scanf("%llu", &x);		
			b = randSrc.raw & 1;
			randSrc.raw >>= 1;
			printf("C %d\n", b);
			block = 1;
			break;
		case 'V':
		case 'v':
			if (!block) {
				printf("E\n");
				break;
			}
			scanf("%llu", &xb);
validate:
			block = 0;
			uint64_t toConfirm = 0;
			if (b == 0) {
				toConfirm = multMod(xb, xb, n);
			}
			if (b == 1) {
				toConfirm = multMod(multMod(xb, xb, n), v, n);
			}
			if (x != toConfirm) {
				printf("E %d\n", t);
				goto reset;
			}
			t--;
			printf("C %d\n", t);
			if (t == 0) {
				n = 0;
				goto reset;
			}
			break;
		case 'C':
		case 'c':
			scanf("%llu %c %llu", &x, &b, &xb);
			b -= '0';
			if (0 > b || b > 1 || n == 0) {
				printf("E %d\n", t);
				goto reset;
			}
			goto validate;
		case 'T':
		case 't':
			printf("C\n");
			return;
		default:
			break;
		}
		mode = 0;
	}
}

// tFunc is the Teodoro function.
void tFunc() {
	int32_t p = 0;
	int32_t q = 0;
	uint64_t n = 0;
	uint64_t v = 0;
	uint64_t s = 0;
	char mode = 0;
	for (;;) {
		scanf(" %c", &mode);
		switch (mode) {
		case 'I':
		case 'i':
			scanf("%d %d", &p, &q);
			n = (int64_t)p * (int64_t)q;
			printf("C %llu\n", n);
			break;
		case 'A':
		case 'a':
			if (n == 0) {
				printf("E\n");
				break;
			}
			s = randIntN(n);
			v = modInverse(multMod(s, s, n), n);
			if (v == 0) {
				printf("E\n");
				break;
			}
			printf("C %llu %llu\n", v, s);
			break;
		case 'F':
		case 'f':
			scanf("%llu", &s);
			if (n == 0 || 2 > s || s >= n) {
				printf("E\n");
				break;
			}
			v = modInverse(multMod(s, s, n), n);
			if (v == 0) {
				printf("E\n");
				break;
			}
			printf("C %llu\n", v);
			break;
		case 'T':
		case 't':
			printf("C\n");
			return;
		default:
			break;
		}
		mode = 0;
	}
}

// eFunc is the Ester function.
void eFunc() {
	uint64_t n = 0;
	uint64_t v = 0;
	uint64_t x = 0;
	uint64_t xb = 0;
	uint64_t x0 = 0;
	uint64_t x1 = 0;
	int b = 0;
	char mode = 0;
	for (;;) {
		scanf(" %c", &mode);
		switch (mode) {
		case 'I':
		case 'i':
			scanf("%llu %llu", &n, &v);
			if (n < 3 || v >= n) {
				printf("E\n");
				break;
			}
			printf("C\n");
			break;
		case 'P':
		case 'p':
			scanf("%d", &b);
			if (0 > b || b > 1) {
				printf("E\n");
				break;
			}
			if (b == 0) {
				xb = randIntN(n);
				x = multMod(xb, xb, n);
			}
			if (b == 1) {
				xb = randIntN(n);
				x = multMod(multMod(xb, xb, n), v, n);
			}
			printf("C %llu %llu\n", x, xb);
			break;
		case 'S':
		case 's':
			scanf("%llu %llu", &x0, &x1);
			uint64_t s = multMod(modInverse(x0, n), x1, n);
			printf("C %llu\n", s);
			break;
		case 'T':
		case 't':
			printf("C\n");
			return;
		default:
			break;
		}
		mode = 0;
	}
}

void pUsage(char *name) {
	printf("Usage: %s <F|P|T|E>\n", name);
}

int main(int argc, char *argv[]) {
	if (argc != 2) {	
		pUsage(argv[0]);
		return 1;
	}
	switch (argv[1][0]) {
	case 'F':	
	case 'f':	
		fFunc();
		break;
	case 'P':
	case 'p':
		pFunc();
		break;
	case 'T':
	case 't':
		tFunc();
		break;
	case 'E':
	case 'e':
		eFunc();	
		break;
	default:
		pUsage(argv[0]);
		return 1;
	}
	return 0;
}
