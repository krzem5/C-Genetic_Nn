#include <stdint.h>
#include <limits.h>
#include <signal.h>



#ifdef assert
#undef assert
#endif
#ifdef NULL
#undef NULL
#endif
#define NULL ((void*)0)
#define bool _Bool
#define true 1
#define false 0
#define assert(x) \
	do{ \
		if (!(x)){ \
			printf("%s: %u (%s): %s: Assertion Failed",__FILE__,__LINE__,__func__,#x); \
			raise(SIGABRT); \
		} \
	} while (0) \




typedef struct _NEURAL_NETWORK* NeuralNetwork;
typedef struct _POPULATION* Population;
typedef void (*TrainFunction)(NeuralNetwork nn);
typedef float (*TestFunction)(NeuralNetwork nn);



struct _NEURAL_NETWORK{
	uint16_t i;
	uint8_t ll;
	uint16_t* l;
	float** w;
	float** b;
	float _e;
	float _lf;
};



struct _POPULATION{
	uint16_t nnl;
	struct _NEURAL_NETWORK* nn;
	uint16_t uc;
	float lr;
	TrainFunction trf;
	TestFunction tf;
};



Population create_genetic_population(uint16_t sz,uint16_t uc,uint16_t i,uint16_t o,TrainFunction trf,TestFunction tf);



void update_population(Population p);



void feedforward_nn(NeuralNetwork nn,float* nn_i,float* nn_o);



void train_nn(NeuralNetwork nn,float* nn_i,float* nn_to,float lr);
