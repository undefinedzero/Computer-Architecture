#include<stdio.h>
#include<time.h>
#include"hash_func.h"

#define MAXLEN 1024
void HMAC(u32* temp, u32* T1, u32* T2, u32* digest){
	int i;
	u32 w0[4],w1[4],w2[4],w3[4];
	for(i=0;i<4;i++){
		w0[i] = temp[i];
		w1[i] = temp[i + 4];
		w2[i] = temp[i + 8];
		w3[i] = temp[i + 12];
	}
	hmac_sha256_run(w0, w1, w2, w3, T1, T2, digest);
}

int main(void){
	time_t start,end,end1;  
	u32 temp[16];
	u32 pwd[16] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
	u32 salt[4] ;//= { 0xdd9c1c17, 0xd1445003, 0x48c4d446, 0xae1770e5};
	u32 check[2];
	int check_r[8];
	u32 j,us;
	int i,num;
	u32 xcheck_val[2];
	u32 out[8];
	int C=1;
    u32 salt_n[16];
    u32 digest[8];
	u32 T1[8];
	u32 T2[8];
	u32 w0[4], w1[4], w2[4], w3[4];
	char addr[1024];
	char s[4];
	FILE * f;
    int rc;
    int notrar5,count;
    unsigned char buf[MAXLEN];
    unsigned char salt_r[16];
    int checkrar5[8] = {0x52, 0x61, 0x72, 0x21, 0x1a, 0x07, 0x01, 0x00};

    printf("请输入待解密文件的地址:");
    scanf("%s",addr);

    //读取rar5数据
	start =time(NULL); 
	f = fopen(addr, "rb");
    fread(buf, 1, MAXLEN, f);

    for(i=0;i<8;i++){
        if (checkrar5[i] != buf[i]){
            notrar5 = 1;
        }
    }
    if(notrar5) {
        printf("not rar!");
        return 0;
    }
    for(i=8;i<MAXLEN;i++) {
        if (buf[i] == 0x00) {
            if (buf[i + 1] == 0x01) {
                count = buf[i + 2];
                for (j = 0; j < 16; j++) {
                    salt_r[j] = buf[j + i + 3];
                }
                for (j = 0; j < 8; j++){
                	check_r[j] = buf[j + i + 19];
                }
                break;
            }
        }
    }

    //解密数据预处理
    salt[0] = salt_r[0]*16777216+salt_r[1]*65536+salt_r[2]*256+salt_r[3]*1;
    salt[1] = salt_r[4]*16777216+salt_r[5]*65536+salt_r[6]*256+salt_r[7]*1;
    salt[2] = salt_r[8]*16777216+salt_r[9]*65536+salt_r[10]*256+salt_r[11]*1;
    salt[3] = salt_r[12]*16777216+salt_r[13]*65536+salt_r[14]*256+salt_r[15]*1;
    check[0] = check_r[0]*16777216+check_r[1]*65536+check_r[2]*256+check_r[3]*1;
    check[1] = check_r[4]*16777216+check_r[5]*65536+check_r[6]*256+check_r[7]*1;
    for(i = 0; i < count; i++){
    	C = C * 2;
    }
    C = C + 32;
    printf("%d\n", C);

    //开始解密
    end1 =time(NULL);
	for (num=0;num<10000;num++){
		printf("%d\n", num);
		us = 0;
		s[0] = num%10+48;
		s[1] = (num/10)%10+48;
		s[2] = (num/100)%10+48;
		s[3] = (num/1000)%10+48;
		for(i=3;i>=0;i--){
			us = us*256 + s[i];
		}
		pwd[0] = us;
		for (i = 0; i<4; i++){
			w0[i] = pwd[i];
			w1[i] = pwd[i + 4];
			w2[i] = pwd[i + 8];
			w3[i] = pwd[i + 12];
		}
		hmac_sha256_pad(w0, w1, w2, w3, T1, T2);
		for(i=0;i<16;i++){
			if(i<4){
				salt_n[i] = salt[i];
			}   
			else if(i==4){
				salt_n[i] = 1;
			}
			else if(i==5){
				salt_n[i] = 0x80000000;
			}
			else if(i==15){
				salt_n[i] = 32*16+32*5;
			}
			else{
				salt_n[i] = 0;
			}
		}

		HMAC(salt_n, T1, T2, digest);
	    for(i=0;i<8;i++){
	        out[i] = digest[i];
	    }

		for(i=0;i<C-1;i++){
			for(j=0;j<16;j++){
	            if(j<=7){
	                temp[j] = digest[j];
	            }
				else if(j==8){
					temp[j] = 0x80000000;
				}
				else if(j==15){
					temp[j] = (64+32)*8;
				}
				else{
					temp[j] = 0;
				}
			}
			HMAC(temp, T1, T2, digest);
			for (j = 0; j < 8; j++){
				out[j] ^= digest[j];
			}
		}
		xcheck_val[0] = out[0]^out[2]^out[4]^out[6];
		xcheck_val[1] = out[1]^out[3]^out[5]^out[7];
		if(xcheck_val[0] == check[0]&&xcheck_val[1] == check[1]){
			break;
		}
	}
	end =time(NULL);  
	printf("password = %d\n", num);
	printf("time1=%f\n",difftime(end1,start)); 
	printf("time=%f\n",difftime(end,start)); 
}
