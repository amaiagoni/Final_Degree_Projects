import schedule
import time
from influxdb import InfluxDBClient
import json
import pickle

# 1. Load trained Machine Learning Models from models.pckl
models = []
with open("models.pckl", "rb") as f:
    while True:
        try:
            models.append(pickle.load(f))
        except EOFError:
            break

# 2. Load database connection configuration parameters
with open('configurationNoiseSourceScript.json') as json_file:
    configuration = json.load(json_file)    
    databaseConfiguration = configuration['databaseConfiguration']
    client = InfluxDBClient(host=databaseConfiguration['host'], port=databaseConfiguration['port'])
    client.switch_database(databaseConfiguration['database'])

# 3. Main function of the script, that will execute every day at 00:00 hours
def get_noise_source():
    retrieved_data = (list(client.query("SELECT * FROM noise_data WHERE class=\'undefined\'")))
    if retrieved_data:
        for data in retrieved_data[0]:
            data_to_predict = [[data["o63"], data["o125"], data["o250"], data["o500"], data["o1000"], data["o2000"], data["o4000"], data["o8000"], data["o16000"]]]
            new_class_knn = models[8].predict(data_to_predict)
            new_class_cart = models[9].predict(data_to_predict)
            if new_class_knn == new_class_cart:
                new_class = new_class_knn[0]
            else:
                new_class = "unpredictable"
            line = ["noise_data key=\"{}\",host=\"{}\",Laeq={},LaeqA={},o31_5={},o63={},o125={},o250={},o500={},o1000={},o2000={},o4000={},o8000={},o16000={},lat={},lon={},alt={},aux_time={},class={} {}\n".format(data["key"], data["host"], data["Laeq"], data["LaeqA"], data["o31_5"], data["o63"], data["o125"], data["o250"], data["o500"], data["o1000"], data["o2000"], data["o4000"], data["o8000"], data["o16000"], data["lat"], data["lon"], data["alt"], data["aux_time"], "\""+new_class+"\"", data["aux_time"])]
            print("Updated: " + str(line))
            client.write_points(line, time_precision='ms', batch_size=1, protocol='line')
    else:
        print("No data to update")
		
# 4. Scheduler for every day at 00:00
schedule.every().day.at("00:00").do(get_noise_source) 

# 5. Main loop of the script, that executes continuously
while True: 
    schedule.run_pending() 
    time.sleep(1) 