from pandas import read_csv, concat, DataFrame
from sklearn.model_selection import train_test_split, cross_val_score, StratifiedKFold
from sklearn.metrics import classification_report, accuracy_score
from sklearn.linear_model import LogisticRegression
from sklearn.tree import DecisionTreeClassifier
from sklearn.neighbors import KNeighborsClassifier
from sklearn.discriminant_analysis import LinearDiscriminantAnalysis
from sklearn.naive_bayes import GaussianNB
from sklearn.svm import SVC
import matplotlib.pyplot as plt
import numpy as np
import json
import pickle


# 1. LOAD DATASET
# 1.1. Number of folds to retrieve from CSV
max_folds = 10
names = ['octave63', 'octave125', 'octave250', 'octave500', 'octave1000', 'octave2000', 'octave4000', 'octave8000', 'octave16000', 'class', 'filename']
total_folds = []

# 1.2. Retrieve all csv files with name zaratamap_dataset_x, where x is the number of the fold (1 to max_folds)
for fold in range(1,max_folds+1):
    url = "zaratamap_dataset_" + str(fold) + ".csv"
    dataset = read_csv(url, names=names)
    dataset = dataset.drop(['filename'], axis=1)
    dataset['class'] = dataset['class'].astype(str)
    total_folds.append(dataset)

# 2. INITIALIZE MODELS
models = []
models.append(('LR', LogisticRegression(solver='liblinear', multi_class='ovr')))
models.append(('LDA', LinearDiscriminantAnalysis()))
models.append(('KNN', KNeighborsClassifier()))
models.append(('CART', DecisionTreeClassifier()))
models.append(('NB', GaussianNB()))
models.append(('SVM', SVC(gamma='auto')))
names = ['LR', 'LDA', 'KNN', 'CART', 'NB', 'SVM']
model_filename = "models.pckl"

# 3. MACHINE LEARNING WITH MANUAL FOLDS

# 3.1. Declare variables
accuracy_model = [[], [], [], [], [], []]
class_accuracies_per_model = [[], [], [], [], [], []]

# 3.2. For each fold...
for k in range(max_folds): 
    # 3.2.1. Concatenate the rest of the folds into one dataset (k_dataset)
    k_dataset = concat([x for i,x in enumerate(total_folds) if i!=k], axis=0, ignore_index=True).values
    # 3.2.2. Obtain train data from k_dataset
    X_train = k_dataset[:,0:9]
    Y_train = k_dataset[:,9]
    # 3.2.3. Obtain test data from the fold (k)
    X_validation = total_folds[k].values[:,0:9]
    Y_validation = total_folds[k].values[:,9]
    # 3.2.4. Train, obtain overall and individual class accuracies on, and store in file each model
    model_index = 0
    for name, model in models:
        model.fit(X_train, Y_train)
        accuracy_model[model_index].append(accuracy_score(Y_validation, model.predict(X_validation), normalize=True)*100)
        cr = classification_report(Y_validation, model.predict(X_validation), output_dict=True)
        class_accuracies_per_model[model_index].append([cr['air_conditioner']['precision'], cr['car_horn']['precision'], cr['children_playing']['precision'], cr['dog_bark']['precision'], cr['drilling']['precision'], cr['engine_idling']['precision'],cr['gun_shot']['precision'], cr['jackhammer']['precision'], cr['siren']['precision'], cr['street_music']['precision']])
        print("Fold " + str(k+1) + " for " + str(name) + ": " + str(accuracy_model[model_index][k]))
        model_index+=1
		
for model in models:
    pickle.dump(model, open(model_filename, 'ab'))
        
# 4. MACHINE LEARNING WITH AUTOMATIC FOLDS (STRATIFIED)

# 4.1. Declare and reset variables
model_index = 0
accuracy_model_stratified = []
class_accuracies_per_model_stratified = [[], [], [], [], [], []]

# 4.2. Obtain the dataset that encompasses all the data in the files (all folds into a unique dataset) and obtain its sample input (X) and output (y) data
all_inclusive_dataset = concat([x for i,x in enumerate(total_folds)], axis=0, ignore_index=True).values
X = all_inclusive_dataset[:,0:9]
y = all_inclusive_dataset[:,9]

# 4.3. Obtain 10 stratified folds
kfold = StratifiedKFold(n_splits=10, random_state=1, shuffle=True)
    
# 4.4. For each model...
for name, model in models:
    # 4.4.1. Automatically obtain its accuracy and store it in file
    cv_results = cross_val_score(model, X, y, cv=kfold, scoring='accuracy')
    pickle.dump(model, open(model_filename, 'ab'))
    print('%s: %f (%f)' % (name, cv_results.mean(), cv_results.std()))
    accuracy_model_stratified.append((cv_results*100).tolist())
    # 4.4.2. Manually obtain individual class accuracies for each fold
    for train_index, test_index in kfold.split(X, y):
        X_train, X_validation = X[train_index], X[test_index]
        Y_train, Y_validation= y[train_index], y[test_index]
        model.fit(X_train, Y_train)
        cr = classification_report(Y_validation, model.predict(X_validation), output_dict=True)
        class_accuracies_per_model_stratified[model_index].append([cr['air_conditioner']['precision'], cr['car_horn']['precision'], cr['children_playing']['precision'], cr['dog_bark']['precision'], cr['drilling']['precision'], cr['engine_idling']['precision'],cr['gun_shot']['precision'], cr['jackhammer']['precision'], cr['siren']['precision'], cr['street_music']['precision']])
    model_index+=1

# 5. STORE OBTAINED ACCURACIES FOR EACH MODEL IN JSON FILE
with open('model_accuracies.json', 'w') as outfile:
    json.dump({"models":names,"mean_accuracy_per_model":accuracy_model,"class_accuracies_per_model":class_accuracies_per_model, "mean_accuracy_per_model_stratified":accuracy_model_stratified,"class_accuracies_per_model_stratified":class_accuracies_per_model_stratified}, outfile)


