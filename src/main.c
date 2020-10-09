#include <genetic_nn.h>
#include <stdio.h>
#include <stdlib.h>



#define LERNING_RATE 0.001f



void train(NeuralNetwork nn){
	float nn_i[]={0,0};
	float nn_o=0;
	train_nn(nn,nn_i,&nn_o,LERNING_RATE);
	*nn_i=1;
	nn_o=1;
	train_nn(nn,nn_i,&nn_o,LERNING_RATE);
	*(nn_i+1)=1;
	nn_o=0;
	train_nn(nn,nn_i,&nn_o,LERNING_RATE);
	*nn_i=0;
	nn_o=1;
	train_nn(nn,nn_i,&nn_o,LERNING_RATE);
}



float test(NeuralNetwork nn){
	float nn_i[]={0,0};
	float nn_o=0;
	feedforward_nn(nn,nn_i,&nn_o);
	float o=nn_o;
	*nn_i=1;
	feedforward_nn(nn,nn_i,&nn_o);
	o+=1-nn_o;
	*(nn_i+1)=1;
	feedforward_nn(nn,nn_i,&nn_o);
	o+=nn_o;
	*nn_i=0;
	feedforward_nn(nn,nn_i,&nn_o);
	o+=1-nn_o;
	// printf("Error: %f (Fitness: %f)\n",o/4,1-o/4);
	return 1-o/4;
}



int main(int argc,const char** argv){
	srand(12345);
	Population p=create_genetic_population(15,1000,2,1,train,test);
	while (true){
		update_population(p);
	}
	return 0;
}
