 /*
  www.aifes.ai
  https://github.com/Fraunhofer-IMS/AIfES_for_Arduino
  Copyright (C) 2020-2023  Fraunhofer Institute for Microelectronic Circuits and Systems.
  All rights reserved.

  AIfES is free software: you can redistribute it and/or modify
  it under the terms of the GNU Affero General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Affero General Public License for more details.

  You should have received a copy of the GNU Affero General Public License
  along with this program.  If not, see <https://www.gnu.org/licenses/>.

  AIfES-Express XOR training 2 layer demo
  --------------------

    Versions:
    1.0.0   Initial version

  AIfES-Express is a simplified API for AIfES, which is directly integrated. So you can simply decide which variant you want to use.
  
  The sketch shows an example of how a neural network is trained from scratch in AIfES-Express using training data.
  As in the example "0_AIfES-Express_XOR_Inference", an XOR gate is mapped here using a neural network. 
  The 4 different states of an XOR gate are fed in as training data here. 
  The network structure in this example is much larger than actually needed for the problem, 
  but is intended to show how easily new layers or different activation functions can be introduced. 
  example has the net structure 2-3-(Sigmoid)-3-(ReLU)-1-(Sigmoid) and combines different activation functions. 
  In this example, the weights are initialized with the Gotrot method. 
  For the training the ADAM Optimizer is used, the SGD Optimizer was commented out. 
  The optimizer runs a batch traininig over a maximum of 500 epochs. 
  The early stopping is activated and stops the training when a desired target loss is reached.
  The calculation is done in float 32.
  
  XOR truth table / training data
  Input    Output
  0   0    0
  0   1    1
  1   0    1
  1   1    0
  
  Tested on:
    Arduino Nano 33 BLE Sense
    Arduino Portenta H7
    
    Arduino Uno and similar devices unfortunately don't provide enough RAM to run this example.

  You can find more AIfES tutorials here:
  https://create.arduino.cc/projecthub/aifes_team
  */

#include <aifes.h>                  // include the AIfES libary

#define DATASETS        4
#define FNN_4_LAYERS    4
#define PRINT_INTERVAL  10

uint32_t global_epoch_counter = 0;

// The print function for the loss. It can be customized.
void printLoss(float loss)
{
    global_epoch_counter = global_epoch_counter + 1;
    Serial.print(F("Epoch: "));
    Serial.print(global_epoch_counter * PRINT_INTERVAL);
    Serial.print(F(" / Loss: "));
    Serial.println(loss,5);
    
}

void setup() {
  Serial.begin(115200); //115200 baud rate (If necessary, change in the serial monitor)
  while (!Serial);
  
  //IMPORTANT
  //AIfES requires random weights for training
  //Here the random seed is generated by the noise of an analog pin
  srand(analogRead(A5));

  Serial.println(F("AIfES-Express XOR training 2 hidden layer demo"));
  Serial.println(F("Type >training< to start"));
 
}

void loop() {

  while(Serial.available() > 0 ){
    String str = Serial.readString();
    if(str.indexOf("training") > -1)       //Keyword "training"
     {
        
        Serial.println(F("AIfES:"));
        Serial.println(F(""));
        Serial.println(F("rand test"));
        Serial.println(rand());

        global_epoch_counter = 0;
        
        uint32_t i;
       
        // -------------------------------- describe the feed forward neural network ----------------------------------
        // neurons each layer
        // FNN_structure[0] = input layer with 2 inputs
        // FNN_structure[1] = hidden (dense) layer with 3 neurons
        // FNN_structure[2] = output (dense) layer with 1 output
        uint32_t FNN_structure[FNN_4_LAYERS] = {2,3,3,1};
    
        // select the activation functions for the dense layer
        AIFES_E_activations FNN_activations[FNN_4_LAYERS - 1];
        FNN_activations[0] = AIfES_E_sigmoid; // Sigmoid for hidden (dense) layer
        FNN_activations[1] = AIfES_E_relu; // Sigmoid for output (dense) layer
        FNN_activations[2] = AIfES_E_sigmoid; // Sigmoid for output (dense) layer
    
        /* possible activation functions
        AIfES_E_relu
        AIfES_E_sigmoid
        AIfES_E_softmax
        AIfES_E_leaky_relu
        AIfES_E_elu
        AIfES_E_tanh
        AIfES_E_softsign
        AIfES_E_linear
        */
    
        // AIfES Express function: calculate the number of weights needed
        uint32_t weight_number = AIFES_E_flat_weights_number_fnn_f32(FNN_structure,FNN_4_LAYERS);
    
        Serial.print(F("Weights: "));
        Serial.println(weight_number);
    
        // FlatWeights array
        float FlatWeights[weight_number];
    
        if(weight_number != sizeof(FlatWeights)/sizeof(float))
        {
            Serial.println(F("Error: number of weights wrong!"));
            return;
        }
    
        // fill the AIfES Express struct
        AIFES_E_model_parameter_fnn_f32 FNN;
        FNN.layer_count = FNN_4_LAYERS;
        FNN.fnn_structure = FNN_structure;
        FNN.fnn_activations = FNN_activations;
        FNN.flat_weights = FlatWeights;
    
        // -------------------------------- create the tensors ----------------------------------
    
        float input_data[] = {0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f};              // Input data
        uint16_t input_shape[] = {DATASETS, (uint16_t)FNN_structure[0]};                    // Definition of the input shape
        aitensor_t input_tensor = AITENSOR_2D_F32(input_shape, input_data);                 // Macro for the simple creation of a float32 tensor. Also usable in the normal AIfES version

        float target_data[] = {0.0f, 1.0f, 1.0f, 0.0f};                                     // Target Data
        uint16_t target_shape[] = {DATASETS, (uint16_t)FNN_structure[FNN_4_LAYERS - 1]};    // Definition of the target shape
        aitensor_t target_tensor = AITENSOR_2D_F32(target_shape, target_data);              // Macro for the simple creation of a float32 tensor. Also usable in the normal AIfES version

        float output_data[DATASETS];                                                        // Output data
        uint16_t output_shape[] = {DATASETS, (uint16_t)FNN_structure[FNN_4_LAYERS - 1]};    // Definition of the output shape
        aitensor_t output_tensor = AITENSOR_2D_F32(output_shape, output_data);              // Macro for the simple creation of a float32 tensor. Also usable in the normal AIfES version
 
        // -------------------------------- init weights settings ----------------------------------
    
        AIFES_E_init_weights_parameter_fnn_f32  FNN_INIT_WEIGHTS;
        FNN_INIT_WEIGHTS.init_weights_method = AIfES_E_init_glorot_uniform;
    
        /* init methods
            AIfES_E_init_uniform
            AIfES_E_init_glorot_uniform
        */
    
        FNN_INIT_WEIGHTS.min_init_uniform = -1; // only for the AIfES_E_init_uniform
        FNN_INIT_WEIGHTS.max_init_uniform = 1;  // only for the AIfES_E_init_uniform
        // -------------------------------- set training parameter ----------------------------------
        AIFES_E_training_parameter_fnn_f32  FNN_TRAIN;
        FNN_TRAIN.optimizer = AIfES_E_adam;
        /* optimizers
            AIfES_E_adam
            AIfES_E_sgd
        */
        FNN_TRAIN.loss = AIfES_E_mse;
        /* loss
            AIfES_E_mse,
            AIfES_E_crossentropy
        */
        FNN_TRAIN.learn_rate = 0.03f;                           // Learning rate is for all optimizers
        FNN_TRAIN.sgd_momentum = 0.0;                           // Only interesting for SGD
        FNN_TRAIN.batch_size = DATASETS;                        // Here a full batch
        FNN_TRAIN.epochs = 500;                                 // Number of epochs
        FNN_TRAIN.epochs_loss_print_interval = PRINT_INTERVAL;  // Print the loss every x times
    
        // Your individual print function
        // it must look like this: void YourFunctionName(float x)
        FNN_TRAIN.loss_print_function = printLoss;

        //You can enable early stopping, so that learning is automatically stopped when a learning target is reached
        FNN_TRAIN.early_stopping = AIfES_E_early_stopping_on;
        /* early_stopping
            AIfES_E_early_stopping_off,
            AIfES_E_early_stopping_on
        */
        //Define your target loss
        FNN_TRAIN.early_stopping_target_loss = 0.009;

        int8_t error = 0;
    
        // -------------------------------- do the training ----------------------------------
        // In the training function, the FNN is set up, the weights are initialized and the training is performed.
        error = AIFES_E_training_fnn_f32(&input_tensor,&target_tensor,&FNN,&FNN_TRAIN,&FNN_INIT_WEIGHTS,&output_tensor);

        error_handling_training(error); 
    
        // -------------------------------- do the inference ----------------------------------
        // AIfES Express function: do the inference
        error = AIFES_E_inference_fnn_f32(&input_tensor,&FNN,&output_tensor);
        
        error_handling_inference(error); 
        
        // -------------------------------- print the results ----------------------------------
   
        uint32_t input_counter = 0;  // Counter to print the inputs/training data
        Serial.println(F(""));
        Serial.println(F("Results:"));
        Serial.println(F("input 1:\tinput 2:\treal output:\tcalculated output:"));
        
        for (i = 0; i < 4; i++) {
          Serial.print (input_data[input_counter]);
          //Serial.print(((float* ) input_tensor.data)[i]); //Alternative print for the tensor
          input_counter++;
          Serial.print (F("\t\t"));
          Serial.print (input_data[input_counter]);
          input_counter++;
          Serial.print (F("\t\t"));
          Serial.print (target_data[i]);
          Serial.print (F("\t\t"));
          Serial.println(output_data[i], 5);
          //Serial.println(((float* ) output_tensor.data)[i], 5); //Alternative print for the tensor
        }

        Serial.println(F(""));
        Serial.println(F("A learning success is not guaranteed"));
        Serial.println(F("The weights were initialized randomly"));
        Serial.println(F("You can repeat the training with >training<"));

     }
    else{
      Serial.println(F("unknown"));
    }
  }

}

void error_handling_training(int8_t error_nr){
  switch(error_nr){
    case 0:
      //Serial.println(F("No Error :)"));
      break;    
    case -1:
      Serial.println(F("ERROR! Tensor dtype"));
      break;
    case -2:
      Serial.println(F("ERROR! Tensor shape: Data Number"));
      break;
    case -3:
      Serial.println(F("ERROR! Input tensor shape does not correspond to ANN inputs"));
      break;
    case -4:
      Serial.println(F("ERROR! Output tensor shape does not correspond to ANN outputs"));
      break;
    case -5:
      Serial.println(F("ERROR! Use the crossentropy as loss for softmax"));
      break;
    case -6:
      Serial.println(F("ERROR! learn_rate or sgd_momentum negative"));
      break;
    case -7:
      Serial.println(F("ERROR! Init uniform weights min - max wrong"));
      break;
    case -8:
      Serial.println(F("ERROR! batch_size: min = 1 / max = Number of training data"));
      break;
    case -9:
      Serial.println(F("ERROR! Unknown activation function"));
      break;
    case -10:
      Serial.println(F("ERROR! Unknown loss function"));
      break;
    case -11:
      Serial.println(F("ERROR! Unknown init weights method"));
      break;
    case -12:
      Serial.println(F("ERROR! Unknown optimizer"));
      break;
    case -13:
      Serial.println(F("ERROR! Not enough memory"));
      break;
    default :
      Serial.println(F("Unknown error"));
  }
}

void error_handling_inference(int8_t error_nr){
  switch(error_nr){
    case 0:
      //Serial.println(F("No Error :)"));
      break;    
    case -1:
      Serial.println(F("ERROR! Tensor dtype"));
      break;
    case -2:
      Serial.println(F("ERROR! Tensor shape: Data Number"));
      break;
    case -3:
      Serial.println(F("ERROR! Input tensor shape does not correspond to ANN inputs"));
      break;
    case -4:
      Serial.println(F("ERROR! Output tensor shape does not correspond to ANN outputs"));
      break;
    case -5:
      Serial.println(F("ERROR! Unknown activation function"));
      break;
    case -6:
      Serial.println(F("ERROR! Not enough memory"));
      break;
    default :
      Serial.println(F("Unknown error"));
  }
}
