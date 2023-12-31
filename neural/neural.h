#ifndef LAYERS_H
#define LAYERS_H

#include <iostream>
#include <string>
#include <functional>

#include "./../linalg/linalg.h"
using std::string;
using std::function;

class Optimizer {

    public:
        string name;
        vector<double> parameters;

        /**
         * @brief Construct a new Optimizer object
         * using presets like "SGD"
         * 
         * NOTE: Currently implemented presets:
         * Vanilla Gradient Decent - ("GD", {learning_rate, useAvgGrad});
         * w.i.p GD w/ Momentum - ("Momentum", {learning_rate, useAvgGrad, 
         * momentum_constant, length});
         * 
         * 
         * @param name 
         * @param parameters vector of parameters for 
         * the specified optimization algorithm. See 
         * above for the format of this vector
         */
        Optimizer(string name, vector<double> parameters);

        // void (*updateAlg)(Tensor* weights, Tensor* accumalatedGrad, int iterSinceLastUpdate, 
        //                   Tensor* cache, vector<Tensor*> queue);


        // because I use the capture list, I have to use the declaration from
        // C++ functional library. This might be worth exploring later on.
        // also might be worth updating other functions to match this style.

        function<void(Tensor*, Tensor*, int, Tensor*, std::vector<Tensor*>)> updateAlg;


};

class Layer {
    
    protected:
    
        Tensor lastInput; // for use in calc grad J_input

    public:
        // some items moved to public for debugging
        // might be work cleaning up later.
        string name;
        vector<int> shape;
        Layer* prevLayer;
        Layer* nextLayer;
        virtual void setPrevLayer(Layer* prevLayer);
        virtual void setNextLayer(Layer* nextLayer);

        /**
         * @brief This method should be called by a Model 
         * object or by this Layer's prevLayer. 
         * 
         * NOTE: If a specific subclass of Layer handles only 
         * Matrices objects - including column vectors - as input 
         * (i.e. Dense layers) then the specific layer 
         * should manually cast the Tensor to a Matrix and 
         * catch for errors
         * 
         * @param input the input Tensor
         */
        virtual void feedForward(const Tensor& input);

        /**
         * @brief This method should be called by a Model object or 
         * this Layer's nextLayer. 
         * 
         * @param delta should be the partial with respect to the 
         * ouput Tensor of the layer backPropagate is being called from. 
         */
        virtual void backPropagate(const Tensor& J_output);

        /**
         * @brief if Layer has weights/biases
         * this method will update them according
         * to the Optimizer that was passed from
         * the Model.
         * 
         * NOTE: accumalated gradient will be RESET
         * after the weights are updated.
         * 
         * @param optim Optimizer that was defined
         * at Model construction and should be used
         * to modify/update the weights according
         * to previous gradient(s)
         * 
         */
        virtual void updateWeights(Optimizer* optim);

        /**
         * @brief prints the details of the layer.
         * for debugging purposes or to see the weights.
         * 
         */
        virtual void print() const;

        
};

class InputLayer : public Layer {
    
    public:
        InputLayer(vector<int> shape);
        void setPrevLayer(Layer* i_prevLayer) override;
        void feedForward(const Tensor& input) override;
        void backPropagate(const Tensor& J_output) override;
        void updateWeights(Optimizer* optim) override;

};

class OutputLayer : public Layer {
    
    public:
        Tensor lastOutput;
        OutputLayer();
        void setPrevLayer(Layer* i_prevLayer) override;
        void setNextLayer(Layer* i_nextLayer) override;
        void feedForward(const Tensor&) override;
        void backPropagate(const Tensor&) override;
        Tensor getLastOutput();

};

class FlattenLayer : public Layer {
    
    public:
        /**
         * @brief Construct a new Flatten Layer object
         * when a model is initialized with a Flatten Layer
         * the Flatten Layer object will adopt the size of the
         * layer behind it. 
         * 
         * NOTE: Flatten will convert the input from an 
         * object of type Tensor to an object of type
         * Matrix. This allows the input to be passed
         * to a Dense Layer.
         * 
         */
        FlattenLayer();
        void feedForward(const Tensor& input) override;
        void backPropagate(const Tensor& J_output) override;
        void setPrevLayer(Layer* i_prevLayer) override;

};

class DenseLayer : public Layer {

    protected:

        //TODO: ENSURE THAT THERE ARE NOT MEM MANAGEMENT OVERSIGHTS WITH POINTERS
        Matrix* weights;
        Matrix* biases;

        // going to go off on a limb and making everything
        // a ptr in the hopes that it makes passing back
        // and forth from optimizer eaier.

        // accumalating gradients
        Matrix* sumJ_weights;
        Matrix* sumJ_biases;
        int iterSincelastUpdate; // to help get average

        // various fields for the optimizer to work with 
        // depending on which algorithm is being used
        Matrix* C_weights;
        Matrix* C_biases;
        vector<Tensor*> Q_weights;
        vector<Tensor*> Q_biases;


    public: 
        DenseLayer(int i_size);
        void setPrevLayer(Layer* i_prevLayer) override;
        void feedForward(const Tensor& input) override;
        void backPropagate(const Tensor& J_output) override;
        void updateWeights(Optimizer* optim) override;
        void print() const override;



};

class ActivationLayer : public Layer {

    protected: 
        string activationName;
        function <Tensor(const Tensor&)> activation;
        function <Tensor(const Tensor&)> activationPrime;

    public: 
        
        /**
         * @brief Construct a new Activation Layer object
         * using presets like "tanh" etc. a later feature 
         * that might be worth adding is adjusting 
         * distribution in last dense layer
         * 
         * NOTE: Currently implemented presets:
         * "sigmoid",
         * "tanh" 
         * 
         * @param name 
         */
        ActivationLayer(string i_name);

        void feedForward(const Tensor& input) override;
        void backPropagate(const Tensor& J_output) override; 



};

class LossFunction {

    protected:
        function <double(const Tensor&, const Tensor&)> loss;
        function <Tensor(const Tensor&, const Tensor&)> lossPrime;

    public:
        string name;
        /**
         * @brief Construct a new Loss Function object.
         * Similar to Activation Layers, you can pick
         * from a set of preset loss functions
         * 
         * NOTE: Currently implemented presets:
         * Mean Squared Error - "MSE",
         * 
         * @param i_name 
         */
        LossFunction(string i_name);
        double getLoss(const Tensor& pred, const Tensor& actual) const;
        Tensor getGradient(const Tensor& pred, const Tensor& actual) const;

};

class Model {

    protected: 

        InputLayer* inputLayer;
        OutputLayer* outputLayer;

        LossFunction* lossFunction;
        Optimizer* optimizer;

    public: 

        /**
         * @brief Construct a new Model object, by 
         * inputting the input shape, hiddenLayers, 
         * LossFunction, and optimizer.
         * 
         * @param shape
         * @param hiddenLayers
         * @param LossFunction
         * @param optimizer
         */
        Model(vector<int> shape, vector<Layer*> hiddenLayers, 
                LossFunction* lossFunction, Optimizer* optim);

        /**
         * @brief Calls feedForward on the input 
         * layer and then returns the final output layer 
         * Tensor. Does not update the gradient.
         * 
         * @param input represents input to the model or 
         * more literally the input to the first layer
         */
        Tensor predict(const Tensor& input);

        /**
         * @brief Runs forward pass, calculates
         * the Loss and negative gradient, then 
         * calls backPropagate on the output 
         * layer to start the backwards pass.
         * 
         * All layers will update their gradient 
         * sum field
         * 
         * @param input 
         * @return returns the loss from the 
         * prediction and target
         */
        double train(const Tensor& input, const Tensor& target);

        /**
         * @brief At the end of a mini-batch,
         * after a gradient has been established
         * this function should be called to update
         * weights
         * 
         */
        void update();

        /**
         * @brief Will print all the details
         * of the model and each layer will
         * print it's own specific details as
         * well.
         * 
         */
        void print();

};

#endif