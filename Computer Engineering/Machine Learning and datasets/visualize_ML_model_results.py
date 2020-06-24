# VISUALIZE OVERALL AND PER CLASS ACCURACIES FOR EACH FOLD IN EACH MODEL

import matplotlib.pyplot as plt
import numpy as np
import json
import sys

# Load data from model_accuracies.json

with open('model_accuracies.json') as json_file:
    model_accuracies = json.load(json_file)
    names = model_accuracies['models']
    accuracy_model = model_accuracies['mean_accuracy_per_model']
    class_accuracies_per_model = model_accuracies['class_accuracies_per_model']
    accuracy_model_stratified = model_accuracies['mean_accuracy_per_model_stratified']
    class_accuracies_per_model_stratified = model_accuracies['class_accuracies_per_model_stratified']

# Visualize machine learning with manual folds (when specified 1 in the command line)
if (len(sys.argv) == 2 and sys.argv[1] == '1'):
    fig, axs = plt.subplots(2, 3,figsize=(14,12))
    fig.suptitle('Accuracies for different Machine Learning models (Manual Folds)')
    y_pos = np.arange(10)
    bar_width = 0.5

    for index, name in enumerate(names):
        row = index//3
        column = index-row*3
        axs[row,column].bar(y_pos, accuracy_model[index], bar_width, color='b')
        for fold_index, fold_results in enumerate(class_accuracies_per_model[index]):
            handle = axs[row,column].scatter(np.full(10, fold_index), np.array(fold_results)*100.0, c=np.arange(10), zorder=2)
        axs[row,column].set_title(str(name))

    for ax in axs.flat:
        ax.set_xticks(y_pos)
        ax.set_xticklabels([1, 2, 3, 4, 5, 6, 7, 8, 9, 10])
    fig.subplots_adjust(bottom=0.2, wspace=0.33)
    axs[1,1].legend(handles=handle.legend_elements()[0],labels=['air_conditioner','car_horn','children_playing','dog_bark','drilling','engine_idling','gun_shot','jackhammer','siren','street_music'], loc='upper center', bbox_to_anchor=(0.5, -0.15), ncol=5)
    plt.show()

# Visualize machine learning with automatic folds (when specified 2 in the command line or by default)
else:
    fig, axs = plt.subplots(2, 3,figsize=(14,12))
    fig.suptitle('Accuracies for different Machine Learning models (Stratified Folds)')
    y_pos = np.arange(10)
    bar_width = 0.5
    
    for index, name in enumerate(names):
        row = index//3
        column = index-row*3
        axs[row,column].bar(y_pos, accuracy_model_stratified[index], bar_width, color='b')
        for fold_index, fold_results in enumerate(class_accuracies_per_model_stratified[index]):
            handle = axs[row,column].scatter(np.full(10, fold_index), np.array(fold_results)*100.0, c=np.arange(10), zorder=2)
        axs[row,column].set_title(str(name))

    for ax in axs.flat:
        ax.set_xticks(y_pos)
        ax.set_xticklabels([1, 2, 3, 4, 5, 6, 7, 8, 9, 10])
    fig.subplots_adjust(bottom=0.2, wspace=0.33)
    axs[1,1].legend(handles=handle.legend_elements()[0],labels=['air_conditioner','car_horn','children_playing','dog_bark','drilling','engine_idling','gun_shot','jackhammer','siren','street_music'], loc='upper center', bbox_to_anchor=(0.5, -0.15), ncol=5)
    plt.show()