#include <genetic_nn.h>
#include <stdlib.h>
#include <stdio.h>



/****************************************/
#define ALL_CLEANUP 1
const char* _s_sig="XYXYXYXY";
const char* _e_sig="ZWZWZWZW";
struct _MEM_BLOCK{
	struct _MEM_BLOCK* p;
	struct _MEM_BLOCK* n;
	void* ptr;
	size_t sz;
	unsigned int ln;
	const char* fn;
	bool f;
} _mem_head={
	NULL,
	NULL,
	NULL,
	0,
	0,
	NULL,
	false
};
void _dump_mem(void* s,size_t sz){
	printf("Memory Dump Of Address 0x%016llx - 0x%016llx (+ %llu):\n",(unsigned long long int)s,(unsigned long long int)s+sz,sz);
	size_t mx_n=8*(((sz+7)>>3)-1);
	unsigned char mx=1;
	while (mx_n>10){
		mx++;
		mx_n/=10;
	}
	char* f=malloc(mx+20);
	sprintf_s(f,mx+20,"0x%%016llx + %% %ullu: ",mx);
	for (size_t i=0;i<sz;i+=8){
		printf(f,(uintptr_t)s,(uintptr_t)i);
		unsigned char j;
		for (j=0;j<8;j++){
			if (i+j>=sz){
				break;
			}
			printf("%02x",*((unsigned char*)s+i+j));
			printf(" ");
		}
		if (j==0){
			break;
		}
		while (j<8){
			printf("   ");
			j++;
		}
		printf("| ");
		for (j=0;j<8;j++){
			if (i+j>=sz){
				break;
			}
			unsigned char c=*((unsigned char*)s+i+j);
			if (c>0x1f&&c!=0x7f){
				printf("%c  ",(char)c);
			}
			else{
				printf("%02x ",c);
			}
		}
		printf("\n");
	}
	free(f);
}
void _valid_mem(unsigned int ln,const char* fn){
	struct _MEM_BLOCK* n=&_mem_head;
	while (true){
		if (n->ptr!=NULL){
			for (unsigned char i=0;i<8;i++){
				if (*((char*)n->ptr+i)!=*(_s_sig+i)){
					printf("ERROR: Line %u (%s): Address 0x%016llx Allocated at Line %u (%s) has been Corrupted (0x%016llx-%u)!\n",ln,fn,((uint64_t)n->ptr+8),n->ln,n->fn,((uint64_t)n->ptr+8),8-i);
					_dump_mem(n->ptr,n->sz+16);
					raise(SIGABRT);
					return;
				}
			}
			for (unsigned char i=0;i<8;i++){
				if (*((char*)n->ptr+n->sz+i+8)!=*(_e_sig+i)){
					printf("ERROR: Line %u (%s): Address 0x%016llx Allocated at Line %u (%s) has been Corrupted (0x%016llx+%llu+%u)!\n",ln,fn,((uint64_t)n->ptr+8),n->ln,n->fn,((uint64_t)n->ptr+8),n->sz,i+1);
					_dump_mem(n->ptr,n->sz+16);
					raise(SIGABRT);
					return;
				}
			}
			if (n->f==true){
				bool ch=false;
				for (size_t i=0;i<n->sz;i++){
					if (*((unsigned char*)n->ptr+i+8)!=0xdd){
						if (ch==false){
							printf("ERROR: Line %u (%s): Detected Memory Change in Freed Block Allocated at Line %u (%s) (0x%016llx):",ln,fn,n->ln,n->fn,(uint64_t)n->ptr);
							ch=true;
						}
						else{
							printf(";");
						}
						printf(" +%llu (%02x)",i,*((unsigned char*)n->ptr+i+8));
					}
				}
				if (ch==true){
					printf("\n");
					_dump_mem(n->ptr,n->sz+16);
					raise(SIGABRT);
				}
			}
		}
		if (n->n==NULL){
			break;
		}
		n=n->n;
	}
}
void _get_mem_block(const void* ptr,unsigned int ln,const char* fn){
	_valid_mem(ln,fn);
	struct _MEM_BLOCK* n=&_mem_head;
	while ((uint64_t)ptr<(uint64_t)n->ptr||(uint64_t)ptr>(uint64_t)n->ptr+n->sz){
		if (n->n==NULL){
			printf("ERROR: Line %u (%s): Unknown Pointer 0x%016llx!\n",ln,fn,(uint64_t)ptr);
			raise(SIGABRT);
			return;
		}
		n=n->n;
	}
	printf("INFO:  Line %u (%s): Found Memory Block Containing 0x%016llx (+%llu) Allocated at Line %u (%s)!\n",ln,fn,(uint64_t)ptr,(uint64_t)ptr-(uint64_t)n->ptr-8,n->ln,n->fn);
	_dump_mem(n->ptr,n->sz+16);
}
bool _all_defined(const void* ptr,size_t e_sz,unsigned int ln,const char* fn){
	_valid_mem(ln,fn);
	struct _MEM_BLOCK* n=&_mem_head;
	while (n->ptr!=(unsigned char*)ptr-8){
		if (n->n==NULL){
			printf("ERROR: Line %u (%s): Unknown Pointer 0x%016llx!\n",ln,fn,(uint64_t)ptr);
			raise(SIGABRT);
			return false;
		}
		n=n->n;
	}
	assert((n->sz/e_sz)*e_sz==n->sz);
	bool e=false;
	for (size_t i=0;i<n->sz;i+=e_sz){
		bool f=true;
		for (size_t j=i;j<i+e_sz;j++){
			if (*((unsigned char*)ptr+j)!=0xcd){
				f=false;
				break;
			}
		}
		if (f==true){
			e=true;
			printf("ERROR: Line %u (%s): Found Uninitialised Memory Section in Pointer Allocated at Line %u (%s): 0x%016llx +%llu -> +%llu!\n",ln,fn,n->ln,n->fn,(uint64_t)ptr,i,i+e_sz);
		}
	}
	if (e==true){
		_dump_mem(n->ptr,n->sz+16);
		return false;
	}
	return true;
}
void _dump_all_mem(){
	struct _MEM_BLOCK* n=&_mem_head;
	while (true){
		if (n->ptr!=NULL){
			printf("Line %u (%s):\n",n->ln,n->fn);
			_dump_mem(n->ptr,n->sz+16);
		}
		if (n->n==NULL){
			break;
		}
		n=n->n;
	}
}
void* _malloc_mem(size_t sz,unsigned int ln,const char* fn){
	_valid_mem(ln,fn);
	if (sz<=0){
		printf("ERROR: Line %u (%s): Negative or Zero Size!\n",ln,fn);
		raise(SIGABRT);
		return NULL;
	}
	struct _MEM_BLOCK* n=&_mem_head;
	while (n->ptr!=NULL){
		if (n->n==NULL){
			n->n=malloc(sizeof(struct _MEM_BLOCK));
			n->n->p=NULL;
			n->n->n=NULL;
			n->n->ptr=NULL;
			n->n->sz=0;
			n->n->ln=0;
			n->n->fn=NULL;
			n->n->f=false;
		}
		n=n->n;
	}
	n->ptr=malloc(sz+16);
	if (n->ptr==NULL){
		printf("ERROR: Line %u (%s): Out of Memory!\n",ln,fn);
		raise(SIGABRT);
		return NULL;
	}
	for (size_t i=0;i<8;i++){
		*((char*)n->ptr+i)=*(_s_sig+i);
		*((char*)n->ptr+sz+i+8)=*(_e_sig+i);
	}
	n->sz=sz;
	n->ln=ln;
	n->fn=fn;
	n->f=false;
	return (void*)((uintptr_t)n->ptr+8);
}
void* _realloc_mem(const void* ptr,size_t sz,unsigned int ln,const char* fn){
	_valid_mem(ln,fn);
	if (ptr==NULL){
		return _malloc_mem(sz,ln,fn);
	}
	assert(sz>0);
	struct _MEM_BLOCK* n=&_mem_head;
	while (n->ptr!=(uint8_t*)ptr-8){
		if (n->n==NULL){
			printf("ERROR: Line %u (%s): Reallocating Unknown Pointer! (%p => %llu)\n",ln,fn,ptr,sz);
			raise(SIGABRT);
			break;
		}
		n=n->n;
	}
	if (n->f==true){
		printf("ERROR: Line %u (%s): Reallocating Freed Pointer! (%p => %llu)\n",ln,fn,ptr,sz);
		raise(SIGABRT);
		return NULL;
	}
	n->ptr=realloc(n->ptr,sz+16);
	if (n->ptr==NULL){
		printf("ERROR: Line %u (%s): Out of Memory! (%p => %llu)\n",ln,fn,ptr,sz);
		raise(SIGABRT);
		return NULL;
	}
	for (size_t i=0;i<8;i++){
		*((unsigned char*)n->ptr+i)=*(_s_sig+i);
		*((unsigned char*)n->ptr+sz+i+8)=*(_e_sig+i);
	}
	for (size_t i=n->sz;i<sz;i++){
		*((unsigned char*)n->ptr+i+8)=0xcd;
	}
	n->sz=sz;
	n->ln=ln;
	n->fn=fn;
	return (void*)((uintptr_t)n->ptr+8);
}
void _free_mem(const void* ptr,unsigned int ln,const char* fn){
	_valid_mem(ln,fn);
	struct _MEM_BLOCK* n=&_mem_head;
	while (n->ptr!=(char*)ptr-8){
		if (n->n==NULL){
			printf("ERROR: Line %u (%s): Freeing Unknown Pointer!\n",ln,fn);
			raise(SIGABRT);
			return;
		}
		n=n->n;
	}
	n->f=true;
	for (size_t i=0;i<n->sz;i++){
		*((unsigned char*)n->ptr+i+8)=0xdd;
	}
#ifdef ALL_CLEANUP
	free(n->ptr);
	n->ptr=NULL;
	n->sz=0;
	n->ln=0;
	n->fn=NULL;
	if (n->p!=NULL){
		n->p->n=n->n;
		if (n->n!=NULL){
			n->n->p=n->p;
		}
		free(n);
	}
#endif
}
#define _str_i(x) #x
#define _str(x) _str_i(x)
#define _concat(a,b) a##b
#define _arg_c_l(...) _,__VA_ARGS__
#define _arg_c_exp(x) x
#define _arg_c_c(_0,_1,N,...) N
#define _arg_c_exp_va(...) _arg_c_exp(_arg_c_c(__VA_ARGS__,1,0))
#define _arg_c(...)  _arg_c_exp_va(_arg_c_l(__VA_ARGS__))
#define _ret_c(t,...) _concat(_return_,t)(__VA_ARGS__)
#define return(...) _ret_c(_arg_c(__VA_ARGS__),__VA_ARGS__)
#define _return_0() \
	do{ \
		_valid_mem(__LINE__,__func__); \
		return; \
	} while(0)
#define _return_1(r) \
	do{ \
		_valid_mem(__LINE__,__func__); \
		return (r); \
	} while(0)
// #undef malloc
// #define malloc(sz) _malloc_mem(sz,__LINE__,__func__)
// #undef realloc
// #define realloc(ptr,sz) _realloc_mem(ptr,sz,__LINE__,__func__)
// #undef free
// #define free(ptr) _free_mem(ptr,__LINE__,__func__)
#define get_mem_block(ptr) _get_mem_block(ptr,__LINE__,__func__)
#define all_defined(ptr,e_sz) _all_defined(ptr,e_sz,__LINE__,__func__)
/****************************************/



float _sigmoid(float x){
	return(1.0f/(1+expf(-5.5f*x)));
}



float _sigmoid_d(float x){
	return(x-x*x);
}



Population create_genetic_population(uint16_t sz,uint16_t uc,uint16_t nn_i,uint16_t nn_o,TrainFunction trf,TestFunction tf){
	assert(sz>1);
	assert(uc>0);
	assert(nn_i>0);
	assert(nn_o>0);
	assert(trf!=NULL);
	assert(tf!=NULL);
	Population o=malloc(sizeof(struct _POPULATION));
	o->nnl=sz;
	o->nn=malloc(sz*sizeof(struct _NEURAL_NETWORK));
	for (uint16_t i=0;i<o->nnl;i++){
		(o->nn+i)->i=nn_i;
		(o->nn+i)->ll=1;
		(o->nn+i)->l=malloc(sizeof(uint16_t));
		*((o->nn+i)->l)=nn_o;
		(o->nn+i)->w=malloc(sizeof(float*));
		*((o->nn+i)->w)=malloc((uint32_t)nn_i*nn_o*sizeof(float));
		for (uint32_t j=0;j<(uint32_t)nn_i*nn_o;j++){
			*(*((o->nn+i)->w)+j)=((float)rand())/RAND_MAX*2-1;
		}
		(o->nn+i)->b=malloc(sizeof(float*));
		*((o->nn+i)->b)=malloc(nn_o*sizeof(float));
		for (uint16_t j=0;j<nn_o;j++){
			*(*((o->nn+i)->b)+j)=((float)rand())/RAND_MAX*2-1;
		}
	}
	o->uc=uc;
	o->trf=trf;
	o->tf=tf;
	return(o);
}



void update_population(Population p){
	float* fl=malloc(p->nnl*sizeof(float));
	uint16_t* id_l=malloc(p->nnl*sizeof(uint16_t));
	for (uint16_t i=0;i<p->nnl;i++){
		for (uint16_t j=0;j<p->uc;j++){
			p->trf(p->nn+i);
		}
		float f=p->tf(p->nn+i);
		if (i==0){
			*fl=f;
			*id_l=i;
		}
		else{
			bool a=false;
			for (uint16_t j=0;j<i;j++){
				if (*(fl+j)<f){
					for (uint16_t k=i;k>j;k--){
						*(fl+k)=*(fl+k-1);
						*(id_l+k)=*(id_l+k-1);
					}
					*(fl+j)=f;
					*(id_l+j)=i;
					a=true;
					break;
				}
			}
			if (a==false){
				*(fl+i)=f;
				*(id_l+i)=i;
			}
		}
	}
	printf("BEST: %f\n",*fl);
	uint16_t ln=(p->nnl<4?2:p->nnl/2);
	float tf=0;
	bool* al=malloc(p->nnl*sizeof(bool));
	for (uint16_t i=0;i<p->nnl;i++){
		if (i<ln){
			tf+=*(fl+i);
			*(al+*(id_l+i))=true;
		}
		else{
			*(al+*(id_l+i))=false;
		}
	}
	uint16_t rm=p->nnl-ln;
	uint16_t j=0;
	for (uint16_t i=ln;i>0;i--){
		uint16_t c=(i>1?(uint16_t)(*(fl+i-1)/tf*(p->nnl-ln)):rm);
		rm-=c;
		for (uint16_t k=0;k<c;k++){
			while (*(al+j)==true){
				j++;
			}
			assert(j<p->nnl);
			for (uint16_t l=0;l<(p->nn+j)->ll;l++){
				free(*((p->nn+j)->w+l));
				free(*((p->nn+j)->b+l));
			}
			if ((p->nn+*(id_l+i-1))->ll>1&&rand()%100==0){
				printf("EXTEND!\n");
				assert(0);
			}
			else if (rand()%300==0){
				printf("ADD!\n");
				assert(0);
			}
			else{
				(p->nn+j)->ll=(p->nn+*(id_l+i-1))->ll;
				(p->nn+j)->l=realloc((p->nn+j)->l,(p->nn+*(id_l+i-1))->ll*sizeof(uint16_t));
				(p->nn+j)->w=realloc((p->nn+j)->w,(p->nn+*(id_l+i-1))->ll*sizeof(float*));
				(p->nn+j)->b=realloc((p->nn+j)->b,(p->nn+*(id_l+i-1))->ll*sizeof(float*));
				for (uint16_t l=0;l<(p->nn+j)->ll;l++){
					*((p->nn+j)->l+l)=*((p->nn+*(id_l+i-1))->l+l);
					uint16_t lenA=(l==0?(p->nn+j)->i:*((p->nn+j)->l+l-1));
					uint16_t lenB=*((p->nn+j)->l+l);
					*((p->nn+j)->w+l)=malloc((uint32_t)lenA*lenB*sizeof(float));
					*((p->nn+j)->b+l)=malloc(lenB*sizeof(float));
					for (uint32_t m=0;m<(uint32_t)lenA*lenB;m++){
						*(*((p->nn+j)->w+l)+m)=*(*((p->nn+*(id_l+i-1))->w+l)+m);
					}
					for (uint16_t m=0;m<lenB;m++){
						*(*((p->nn+j)->b+l)+m)=*(*((p->nn+*(id_l+i-1))->b+l)+m);
					}
				}
			}
			j++;
		}
	}
	free(fl);
	free(al);
	return();
}



void feedforward_nn(NeuralNetwork nn,float* nn_i,float* nn_o){
	float* tmp=nn_i;
	for (uint16_t i=0;i<nn->ll;i++){
		uint16_t lenA=(i==0?nn->i:*(nn->l+i-1));
		uint16_t lenB=*(nn->l+i);
		float* ntmp=(i<nn->ll-1?malloc(lenB*sizeof(float)):nn_o);
		if (i==nn->ll-1){
			assert(ntmp==nn_o);
		}
		for (uint16_t j=0;j<lenB;j++){
			*(ntmp+j)=*(*(nn->b+i)+j);
			for (uint16_t k=0;k<lenA;k++){
				*(ntmp+j)+=(*(*(nn->w+i)+(uint32_t)k*lenB+j))*(*(tmp+k));
			}
			*(ntmp+j)=_sigmoid(*(ntmp+j));
		}
		if (tmp!=nn_i){
			free(tmp);
		}
		if (i<nn->ll-1){
			tmp=ntmp;
		}
	}
	return();
}



void train_nn(NeuralNetwork nn,float* nn_i,float* nn_to,float lr){
	float** ol=malloc((nn->ll+1)*sizeof(float*));
	float* e=malloc((*(nn->l+nn->ll-1))*sizeof(float));
	*ol=nn_i;
	for (uint16_t i=0;i<nn->ll;i++){
		uint16_t lenA=(i==0?nn->i:*(nn->l+i-1));
		uint16_t lenB=*(nn->l+i);
		*(ol+i+1)=malloc(lenB*sizeof(float));
		for (uint16_t j=0;j<lenB;j++){
			*(*(ol+i+1)+j)=*(*(nn->b+i)+j);
			for (uint16_t k=0;k<lenA;k++){
				*(*(ol+i+1)+j)+=(*(*(nn->w+i)+(uint32_t)k*lenB+j))*(*(*(ol+i)+k));
			}
			*(*(ol+i+1)+j)=_sigmoid(*(*(ol+i+1)+j));
			if (i==nn->ll-1){
				*(e+j)=*(nn_to+j)-*(*(ol+i+1)+j);
			}
		}
	}
	uint16_t i=nn->ll-1;
	while (true){
		uint16_t lenA=(i==0?nn->i:*(nn->l+i-1));
		uint16_t lenB=*(nn->l+i);
		float* g=malloc(lenB*sizeof(float));
		for (uint16_t j=0;j<lenB;j++){
			*(g+j)=_sigmoid_d(*(*(ol+i+1)+j))*(*(e+j))*lr;
			*(*(nn->b+i)+j)+=*(g+j);
		}
		for (uint16_t j=0;j<lenA;j++){
			for (uint16_t k=0;k<lenB;k++){
				*(*(nn->w+i)+(uint32_t)j*lenB+k)+=*(g+k)*(*(*(ol+i)+j));
			}
		}
		free(g);
		if (i==0){
			free(e);
			break;
		}
		float* ne=malloc(lenA*sizeof(float));
		for (uint16_t j=0;j<lenA;j++){
			*(ne+j)=0;
			for (uint16_t k=0;k<lenB;k++){
				*(ne+j)+=*(e+k)*(*(*(nn->w+i)+(uint32_t)j*lenB+k));
			}
		}
		free(e);
		e=ne;
		i--;
	}
	for (i=1;i<nn->ll+1;i++){
		free(*(ol+i));
	}
	free(ol);
	return();
}
