from scipy import signal
import numpy as np
from math import sqrt
from pydub import AudioSegment
import os.listdir, os.path
from pandas import read_csv
import sys

# Function to calculate RMS of the array passed as parameter (array that represents an audio segment)
def rms(num):
    sum = 0
    for n in num:
        sum+=(n*n)
    return (sqrt(sum/num.size))

# Coefficients for A-Weighting and Octave band IIR filters
sosAWeighting = [[0.2343,0.4686,0.2343,1,-1.8939,0.89516],[1,-2.0001,1.0001,1,-1.9946,0.99462],[1,-1.9999,0.99986,1,-0.22456,0.012607]]
sos31_5 = [[0.0014512,-3.7657e-11,-0.0014512,1,-1.9982,0.99821],[0.0014512,0.0029025,0.0014512,1,-1.9955,0.99551],[0.0014512,-0.0029025,0.0014512,1,-2.0005,1.0005]]
sos63 = [[0.0028997,-4.6561e-10,-0.0028997,1,-1.994,0.99403],[0.0028997,0.0057993,0.0028996,1,-1.9961,0.99623],[0.0028997,-0.0057993,0.0028996,1,-1.9981,0.99814]]
sos125 = [[0.0057424,-9.2208e-10,-0.0057424,1,-1.9883,0.98854],[0.0057424,0.011485,0.0057423,1,-1.9921,0.99256],[0.0057424,-0.011485,0.0057423,1,-1.9958,0.99594]]
sos250 = [[0.011441,-1.8372e-09,-0.011441,1,-1.9761,0.9772],[0.011441,0.022882,0.011441,1,-1.9832,0.98518],[0.011441,-0.022882,0.011441,1,-1.9913,0.99191]]
sos500 = [[0.02271,-3.6467e-09,-0.02271,1,-1.9507,0.95492],[0.02271,0.04542,0.02271,1,-1.9628,0.97058],[0.02271,-0.04542,0.02271,1,-1.9816,0.98389]]
sos1000 = [[0.04475,-7.1858e-09,-0.044751,1,-1.8954,0.91178],[0.04475,0.089501,0.04475,1,-1.9116,0.94211],[0.04475,-0.089501,0.04475,1,-1.9588,0.96799]]
sos2000 = [[0.08697,-1.3965e-08,-0.086971,1,-1.7681,0.83067],[0.08697,0.17394,0.086969,1,-1.7703,0.88818],[0.08697,-0.17394,0.086969,1,-1.9008,0.93674]]
sos4000 = [[0.16487,-2.6475e-08,-0.16488,1,-1.4571,0.68551],[0.16487,0.32975,0.16487,1,-1.3581,0.79336],[0.16487,-0.32975,0.16487,1,-1.7372,0.87536]]
sos8000 = [[0.30049,-4.8251e-08,-0.30049,1,-0.68723,0.44235],[0.30049,0.60098,0.30049,1,-0.22883,0.66257],[0.30049,-0.60098,0.30049,1,-1.2517,0.74954]]
sos16000 = [[0.524,-8.4141e-08,-0.52401,1,0.85657,0.047587],[0.524,-1.048,0.524,1,-0.076869,0.39503],[0.524,1.048,0.524,1,1.8159,0.8489]]

# Audio division in milliseconds (1 second)
slicesize = 1000

# 1. Retrieve the fold number passed as parameter when calling the script
fold_number = int(sys.argv[1])

# 2. Open the metadata file provided by the UrbanSound8K dataset and extract the rows that correspond to the selected fold
url = "D:/UrbanSound8K/metadata/UrbanSound8K.csv"
dataset = read_csv(url)
fold = dataset.loc[dataset['fold'] == fold_number]

# 3. Retrieve all audio file paths in the UrbanSound 8K dataset selected fold folder and calculate its total number
all_files = [name for name in os.listdir(os.path.abspath("D:/UrbanSound8K/audio/fold" + str(fold_number))) if name.endswith(".wav")]
total_size = len(all_files)

# 4. Declare an array for storing the data recordings extracted from the audio excerpts
rmsValues = []

# 5. For each audio file...
for filename in all_files:
    try:
        if (all_files.index(filename) % 50 == 0):
            print(str(all_files.index(filename)) + " of " + str(total_size))
			
		# 5.1. Load the wav file
        newAudio = AudioSegment.from_wav("D:/UrbanSound8K/audio/fold" + str(fold_number) + "/" + filename)
		
		# 5.2. Calculate its highest digitalization value, it will serve for ponderation later
        highestValue = np.amax(newAudio.get_array_of_samples())

		# 5.3. Calculate the number of 1 second slices in the audio recording, and for each of them...
        for i in range(0, len(newAudio)//slicesize):
			# 5.3.1. Extract the slice from the original audio recording
            slice = newAudio[i*1000:i*1000+1000]
			# 5.3.2. Pass the slice through the octave filters and calculate their corresponding RMS value in a 0 to 3.3 range
            row = [(rms(signal.sosfilt(sos63, slice.get_array_of_samples()))/highestValue)*3.3,
                              (rms(signal.sosfilt(sos125, slice.get_array_of_samples()))/highestValue)*3.3,
                              (rms(signal.sosfilt(sos250, slice.get_array_of_samples()))/highestValue)*3.3,
                              (rms(signal.sosfilt(sos500, slice.get_array_of_samples()))/highestValue)*3.3,
                              (rms(signal.sosfilt(sos1000, slice.get_array_of_samples()))/highestValue)*3.3,
                              (rms(signal.sosfilt(sos2000, slice.get_array_of_samples()))/highestValue)*3.3,
                              (rms(signal.sosfilt(sos4000, slice.get_array_of_samples()))/highestValue)*3.3,
                              (rms(signal.sosfilt(sos8000, slice.get_array_of_samples()))/highestValue)*3.3,
                              (rms(signal.sosfilt(sos16000, slice.get_array_of_samples()))/highestValue)*3.3,
                              (fold.loc[fold['slice_file_name'] == filename, 'class']).values[0],
							   filename]
			# 5.3.3. Append the results, containing the filename and the class extracted from the metadata file, to the main storage array
            rmsValues.append(row)
    except FileNotFoundError:
        print("Error in " + filename)
        pass

# 6. Store all the data recordings into a csv called zaratamap_dataset_x.csv, where x is the number of the folder being processed
np.savetxt('zaratamap_dataset_' + str(fold_number) + '.csv', np.asarray(rmsValues), delimiter=",", fmt="%s")

                          

        

