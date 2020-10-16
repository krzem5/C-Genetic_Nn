#include <genetic_nn.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>



#define INITIAL_EQUALIZER 1
#define EQUALIZER_MLT 0.75f



float _sigmoid(float x){
	return 1.0f/(1+expf(-5.5f*x));
}



float _sigmoid_d(float x){
	return x-x*x;
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
		(o->nn+i)->_e=INITIAL_EQUALIZER;
		(o->nn+i)->_lf=INFINITY;
	}
	o->uc=uc;
	o->trf=trf;
	o->tf=tf;
	return o;
}



void update_population(Population p){
	float* fl=malloc(p->nnl*sizeof(float));
	float* scl=malloc(p->nnl*sizeof(float));
	uint16_t* id_l=malloc(p->nnl*sizeof(uint16_t));
	for (uint16_t i=0;i<p->nnl;i++){
		for (uint16_t j=0;j<p->uc;j++){
			p->trf(p->nn+i);
		}
		float f=p->tf(p->nn+i);
		if (isinf((p->nn+i)->_lf)==true){
			(p->nn+i)->_lf=f;
		}
		float sc=f*(1+5*(f-(p->nn+i)->_lf))*(1+(p->nn+i)->_e);
		(p->nn+i)->_lf=f;
		if (i==0){
			*fl=f;
			*scl=sc;
			*id_l=i;
		}
		else{
			bool a=false;
			for (uint16_t j=0;j<i;j++){
				if (*(scl+j)<sc){
					for (uint16_t k=i;k>j;k--){
						*(fl+k)=*(fl+k-1);
						*(scl+k)=*(scl+k-1);
						*(id_l+k)=*(id_l+k-1);
					}
					*(fl+j)=f;
					*(scl+j)=sc;
					*(id_l+j)=i;
					a=true;
					break;
				}
			}
			if (a==false){
				*(fl+i)=f;
				*(scl+i)=sc;
				*(id_l+i)=i;
			}
		}
	}
	printf("BEST: %f (SCORE: %f, ID: %hu, LC: %hhu)\n",*fl,*scl,*id_l,(p->nn+*id_l)->ll);
	free(scl);
	uint16_t ln=(p->nnl<4?2:p->nnl/2);
	float tf=0;
	bool* al=malloc(p->nnl*sizeof(bool));
	for (uint16_t i=0;i<p->nnl;i++){
		if (i<ln){
			tf+=*(fl+i);
			*(al+*(id_l+i))=true;
			(p->nn+*(id_l+i))->_e*=EQUALIZER_MLT;
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
			for (uint8_t l=0;l<(p->nn+j)->ll;l++){
				free(*((p->nn+j)->w+l));
				free(*((p->nn+j)->b+l));
			}
			if ((p->nn+*(id_l+i-1))->ll>1&&rand()%100==0){
				uint8_t nl=rand()%((p->nn+*(id_l+i-1))->ll-1);
				while (*((p->nn+*(id_l+i-1))->l+nl)==UINT8_MAX){
					nl=rand()%((p->nn+*(id_l+i-1))->ll-1);
				}
				(p->nn+j)->ll=(p->nn+*(id_l+i-1))->ll;
				(p->nn+j)->l=realloc((p->nn+j)->l,(p->nn+*(id_l+i-1))->ll*sizeof(uint16_t));
				(p->nn+j)->w=realloc((p->nn+j)->w,(p->nn+*(id_l+i-1))->ll*sizeof(float*));
				(p->nn+j)->b=realloc((p->nn+j)->b,(p->nn+*(id_l+i-1))->ll*sizeof(float*));
				for (uint8_t l=0;l<(p->nn+j)->ll;l++){
					*((p->nn+j)->l+l)=*((p->nn+*(id_l+i-1))->l+l)+(l==nl?1:0);
					uint16_t lenA=(l==0?(p->nn+j)->i:*((p->nn+j)->l+l)+(l-1==nl?1:0));
					uint16_t lenB=*((p->nn+j)->l+l);
					*((p->nn+j)->w+l)=malloc((uint32_t)lenA*lenB*sizeof(float));
					*((p->nn+j)->b+l)=malloc(lenB*sizeof(float));
					if (l==nl){
						for (uint16_t m=0;m<lenA;m++){
							for (uint16_t n=0;n<lenB;n++){
								if (n<lenB-1){
									*(*((p->nn+j)->w+l)+(uint32_t)m*lenB+n)=*(*((p->nn+*(id_l+i-1))->w+l)+(uint32_t)m*(lenB-1)+n);
								}
								else{
									*(*((p->nn+j)->w+l)+(uint32_t)m*lenB+n)=((float)rand())/RAND_MAX*2-1;
								}
							}
						}
					}
					else if (l-1==nl){
						for (uint16_t m=0;m<lenA;m++){
							for (uint16_t n=0;n<lenB;n++){
								if (m<lenA-1){
									*(*((p->nn+j)->w+l)+(uint32_t)m*lenB+n)=*(*((p->nn+*(id_l+i-1))->w+l)+(uint32_t)m*lenB+n);
								}
								else{
									*(*((p->nn+j)->w+l)+(uint32_t)m*lenB+n)=((float)rand())/RAND_MAX*2-1;
								}
							}
						}
					}
					else{
						for (uint32_t m=0;m<(uint32_t)lenA*lenB;m++){
							*(*((p->nn+j)->w+l)+m)=*(*((p->nn+*(id_l+i-1))->w+l)+m);
						}
					}
					for (uint16_t m=0;m<lenB;m++){
						*(*((p->nn+j)->b+l)+m)=*(*((p->nn+*(id_l+i-1))->b+l)+m);
					}
				}
				(p->nn+j)->_e=INITIAL_EQUALIZER;
				(p->nn+j)->_lf=INFINITY;
				printf("EXTEND!\n");
				// assert(0);
			}
			else if ((p->nn+*(id_l+i-1))->ll>1&&rand()%100==0){
				printf("RETRACT!\n");
				assert(0);
			}
			else if (rand()%500==0){
				assert((p->nn+*(id_l+i-1))->ll<UINT8_MAX);
				uint8_t nl=rand()%((p->nn+*(id_l+i-1))->ll);
				#define NEW_LEN 1
				(p->nn+j)->ll=(p->nn+*(id_l+i-1))->ll+1;
				(p->nn+j)->l=realloc((p->nn+j)->l,(p->nn+*(id_l+i-1))->ll*sizeof(uint16_t));
				(p->nn+j)->w=realloc((p->nn+j)->w,(p->nn+*(id_l+i-1))->ll*sizeof(float*));
				(p->nn+j)->b=realloc((p->nn+j)->b,(p->nn+*(id_l+i-1))->ll*sizeof(float*));
				uint8_t s=0;
				for (uint8_t l=0;l<(p->nn+j)->ll;l++){
					if (l==nl){
						*((p->nn+j)->l+l)=NEW_LEN;
						uint16_t lenA=(l==0?(p->nn+j)->i:*((p->nn+j)->l+l-1));
						uint16_t lenB=*((p->nn+j)->l+l);
						*((p->nn+j)->w+l)=malloc((uint32_t)lenA*lenB*sizeof(float));
						*((p->nn+j)->b+l)=malloc(lenB*sizeof(float));
						for (uint32_t m=0;m<(uint32_t)lenA*lenB;m++){
							*(*((p->nn+j)->w+l)+m)=((float)rand())/RAND_MAX*2-1;
						}
						for (uint16_t m=0;m<lenB;m++){
							*(*((p->nn+j)->b+l)+m)=((float)rand())/RAND_MAX*2-1;
						}
						s=1;
						continue;
					}
					if (s==1){
						*((p->nn+j)->l+l)=*((p->nn+*(id_l+i-1))->l+l-1);
						uint16_t lenA=NEW_LEN;
						uint16_t lenB=*((p->nn+j)->l+l);
						*((p->nn+j)->w+l)=malloc((uint32_t)lenA*lenB*sizeof(float));
						*((p->nn+j)->b+l)=malloc(lenB*sizeof(float));
						for (uint32_t m=0;m<(uint32_t)lenA*lenB;m++){
							*(*((p->nn+j)->w+l)+m)=((float)rand())/RAND_MAX*2-1;
						}
						for (uint16_t m=0;m<lenB;m++){
							*(*((p->nn+j)->b+l)+m)=((float)rand())/RAND_MAX*2-1;
						}
						s=2;
						continue;
					}
					*((p->nn+j)->l+l)=*((p->nn+*(id_l+i-1))->l+l-s+1);
					uint16_t lenA=(s!=0&&nl==l-1?NEW_LEN:(l==0?(p->nn+j)->i:*((p->nn+j)->l+l-s+1)));
					uint16_t lenB=*((p->nn+j)->l+l);
					*((p->nn+j)->w+l)=malloc((uint32_t)lenA*lenB*sizeof(float));
					*((p->nn+j)->b+l)=malloc(lenB*sizeof(float));
					for (uint32_t m=0;m<(uint32_t)lenA*lenB;m++){
						*(*((p->nn+j)->w+l)+m)=*(*((p->nn+*(id_l+i-1))->w+l-s+1)+m);
					}
					for (uint16_t m=0;m<lenB;m++){
						*(*((p->nn+j)->b+l)+m)=*(*((p->nn+*(id_l+i-1))->b+l-s+1)+m);
					}
				}
				(p->nn+j)->_e=INITIAL_EQUALIZER;
				(p->nn+j)->_lf=INFINITY;
			}
			else if ((p->nn+*(id_l+i-1))->ll>1&&rand()%500==0){
				printf("REMOVE!\n");
				assert(0);
			}
			else{
				(p->nn+j)->ll=(p->nn+*(id_l+i-1))->ll;
				(p->nn+j)->l=realloc((p->nn+j)->l,(p->nn+*(id_l+i-1))->ll*sizeof(uint16_t));
				(p->nn+j)->w=realloc((p->nn+j)->w,(p->nn+*(id_l+i-1))->ll*sizeof(float*));
				(p->nn+j)->b=realloc((p->nn+j)->b,(p->nn+*(id_l+i-1))->ll*sizeof(float*));
				for (uint8_t l=0;l<(p->nn+j)->ll;l++){
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
				(p->nn+j)->_e=(p->nn+*(id_l+i-1))->_e;
				(p->nn+j)->_lf=(p->nn+*(id_l+i-1))->_lf;
			}
			j++;
		}
	}
	free(fl);
	free(al);
}



void feedforward_nn(NeuralNetwork nn,float* nn_i,float* nn_o){
	float* tmp=nn_i;
	for (uint8_t i=0;i<nn->ll;i++){
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
}



void train_nn(NeuralNetwork nn,float* nn_i,float* nn_to,float lr){
	float** ol=malloc((nn->ll+1)*sizeof(float*));
	float* e=malloc((*(nn->l+nn->ll-1))*sizeof(float));
	*ol=nn_i;
	for (uint8_t i=0;i<nn->ll;i++){
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
	uint8_t i=nn->ll-1;
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
}
