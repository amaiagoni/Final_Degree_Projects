from django.shortcuts import render, redirect
from django.conf import settings
from influxdb import InfluxDBClient
from collections import OrderedDict
from django.contrib.auth.forms import AuthenticationForm, PasswordChangeForm
from .forms import UserCreationForm, UserCreationFormWithName, FilterForm, EmailChangeForm, UploadImageForm
from django.contrib.auth.decorators import login_required, user_passes_test
from django.contrib.auth import login as djangologin, logout as djangologout, update_session_auth_hash
from django.contrib.auth.models import Group, User
import math
import itertools
from .models import Street, FilterModelForm, Filter, ProfilePicture, City
import json
from django.http import JsonResponse, HttpResponse
import datetime, calendar
from numpy import sign
from django.utils.translation import gettext
from django.urls import resolve
import numpy as np
from django.core.files.storage import FileSystemStorage
import time
from scipy import signal

def get_influxdb_client():
    """Returns an ``InfluxDBClient`` instance."""
    client = InfluxDBClient(
        settings.INFLUXDB_HOST,
        settings.INFLUXDB_PORT,
        settings.INFLUXDB_USERNAME,
        settings.INFLUXDB_PASSWORD,
        settings.INFLUXDB_DATABASE,
        timeout=getattr(settings, 'INFLUXDB_TIMEOUT', 10),
        ssl=getattr(settings, 'INFLUXDB_SSL', False),
        verify_ssl=getattr(settings, 'INFLUXDB_VERIFY_SSL', False),
    )
    client.switch_database("TestData")
    return client

def index(request):
    context = {'profile_picture': get_profile_picture(request)}
    return render(request, 'zaratamap_app/home.html', context)

def table(request, filter=''):
    retrieved = False
    sub_dict = {"filter": [], "points": [], "geoJsonPointsList": []}
    try:
        retrieved_filter = User.objects.get(username=request.user.username).filter_set.get(name=filter)
        filter_form = FilterModelForm(instance=retrieved_filter)
        sub_dict = {key:value for key, value in filter_form.initial.items() if key not in {"id", "name", "filter_date"}}
        data = get_filtered_data_from_database(sub_dict)
        data["geoJsonStreets"] = get_noise_map(data)
        sub_dict.update(data)
        retrieved = True
    except Filter.DoesNotExist:
        sub_dict = {}
    try:
        get_influxdb_client().ping()
        centralPoint = [43.270200001993764, -2.9456500000716574]
        zoom = 15
        keys = (list((["date", "time", "lat", "lon", "alt", "Laeq", "LaeqA", "o63", "o125", "o250", "o500", "o1000", "o2000", "o4000", "o8000", "o16000", "class"])))
        for key, value in sub_dict.items():
            if key not in {"vehicles", "points", "geoJsonPointsList", "geoJsonStreets"}:
                sub_dict[key] = str(value)
        sub_dict = str(sub_dict).replace("None", "")
        context = {'profile_picture': get_profile_picture(request), 'retrieved': retrieved, 'column_names':keys, 'points': {}, 'geoJsonStreets': {}, 'zoom': zoom, 'center': centralPoint, 'geoJsonPoints': {}, 'initialValues': json.dumps(sub_dict)}
        return render(request, 'zaratamap_app/data-table.html', context)
    except Exception as e:
        context = {'profile_picture': get_profile_picture(request)}
        return render(request, 'zaratamap_app/server-down.html', context)

colors = {(0,35): "#FFFFFF", (35,40):"#A0BABF", (40,45):"#B8D6D1", (45,50):"#CEE4CC", (50,55):"#E2F2BF",
          (55,60): "#F3C683", (60,65):"#E87E4D", (65,70):"#CD463E", (70,75):"#A11A4D", (75,80):"#75085C", (80, float("inf")):"#430A4A"}

def table_row(request):
    data = { "points": [], "geoJsonPointsList": []}
    if request.method == 'POST':
        form = FilterForm(request.POST)
        if form.is_valid():
            vehicles = "{\"vehicles\":["
            if (request.POST.getlist("vehicle_type") and
                    request.POST.getlist("vehicleID1") and request.POST.getlist("vehicleID2") and
                    request.POST.getlist("vehicleID3") and request.POST.getlist("vehicleID4") and
                    len(request.POST.getlist("vehicle_type")) == len(request.POST.getlist("vehicleID1")) and
                    len(request.POST.getlist("vehicleID1")) == len(request.POST.getlist("vehicleID2")) and
                    len(request.POST.getlist("vehicleID2")) == len(request.POST.getlist("vehicleID3")) and
                    len(request.POST.getlist("vehicleID3")) == len(request.POST.getlist("vehicleID4"))):
                for i in range(0, len(request.POST.getlist("vehicle_type"))):
                    vehicles = vehicles + "\"" + (str(request.POST.getlist("vehicle_type")[i]) +
                                                  str(request.POST.getlist("vehicleID1")[i]) +
                                                  str(request.POST.getlist("vehicleID2")[i]) +
                                                  str(request.POST.getlist("vehicleID3")[i]) +
                                                  str(request.POST.getlist("vehicleID4")[i]) + "\",")
                vehicles = vehicles[:-1]
                vehicles = vehicles + "]}"
                form.cleaned_data["vehicles"] = vehicles
            data = get_filtered_data_from_database(form.cleaned_data)
            data["geoJsonStreets"] = get_noise_map(data)
        else:
            print(form.errors)
    return JsonResponse(data)

def get_filtered_data_from_database(filter_dict):
    query = "SELECT \"class\", \"time\", \"lat\", \"lon\", \"alt\", \"Laeq\", \"LaeqA\", \"o63\", \"o125\", \"o250\", \"o500\", \"o1000\", \"o2000\", \"o4000\", \"o8000\", \"o16000\" FROM noise_data"
    if ("refresh-time" in filter_dict):
        query = query + " WHERE aux_time>=" + str(
            int(round(time.time() * 1000)) - int(filter_dict["refresh-time"]) * 60000)
        if filter_dict["refresh-city"] != "All":
            query = query + " AND ("
            street = City.objects.get(name=filter_dict["refresh-city"])
            query = query + "(lat>" + str(street.rectangle_lower_lat) + " AND lat<" + str(
                street.rectangle_upper_lat) + " AND lon>" + str(
                street.rectangle_lower_lon) + " AND lon<" + str(street.rectangle_upper_lon) + "))"
    else:
        firstFilter = True
        for key, value in filter_dict.items():
            if value != None and value != "" and value != False and \
                    key != "lon_int" and key != "lon_dec" and key != "lat_dec" and key != "km_int" and key != "km_dec":
                if firstFilter:
                    query = query + " WHERE "
                    firstFilter = False
                else:
                    query = query + " AND "
                if key == "noise_from":
                    query = query + "LaeqA>" + str(value)
                elif key == "noise_to":
                    query = query + "LaeqA<" + str(value)
                elif key == "above_max_night":
                    if filter_dict["above_max_day"] == True:
                        query = query + "("
                    query = query + "(LaeqA>50 AND (((aux_time-0)%86400)<25201 OR ((aux_time-82800)%86400)<3600))"
                elif key == "above_max_day":
                    if filter_dict["above_max_night"] == True:
                        query = query[:-5] + " OR "
                    query = query + "(LaeqA>55 AND ((aux_time-25200)%86400)<57601)"
                    if filter_dict["above_max_night"] == True:
                        query = query + ")"
                elif key == "date_from":
                    query = query + "aux_time>=" + str(calendar.timegm(datetime.datetime.strptime(str(filter_dict["date_from"]), '%Y-%m-%d').utctimetuple()))
                elif key == "date_to":
                    query = query + "aux_time<=" + str(calendar.timegm(datetime.datetime.strptime(str(filter_dict["date_to"]), '%Y-%m-%d').utctimetuple()) + 86399)
                elif key == "time_from":
                    if filter_dict["time_to"] != None:
                        query = query + "((aux_time-" + str(
                            value.hour * 3600 + value.minute * 60 +
                            value.second) + ")%86400)<" + str(filter_dict["time_to"].hour * 3600 + filter_dict["time_to"].minute * 60 +
                                                              filter_dict["time_to"].second - (filter_dict["time_from"].hour * 3600 + filter_dict["time_from"].minute * 60 +
                                                                                               filter_dict["time_from"].second) + 1)
                    else:
                        query = query + "((aux_time-" + str(
                            value.hour * 3600 + value.minute * 60 +
                            value.second) + ")%86400)<" + str(
                            86400 - (
                                    filter_dict["time_from"].hour * 3600 + filter_dict[
                                "time_from"].minute * 60 +
                                    filter_dict["time_from"].second))
                elif key == "time_to":
                    if filter_dict["time_from"] != None:
                        query = query[:-4]
                    else:
                        query = query + "((aux_time-" + str(
                            value.hour * 3600 + value.minute * 60 +
                            value.second) + ")%86400)>" + str(
                            filter_dict["time_to"].hour * 3600 + filter_dict[
                                "time_to"].minute * 60 +
                            filter_dict["time_to"].second)
                elif key == "lat_int":
                    if filter_dict["lon_int"] != None:
                        longitude = int(filter_dict['lon_int']) + sign(filter_dict['lon_int'])*(int((filter_dict['lon_dec'])) / (10 ** len(str(filter_dict['lon_dec']))))
                        latitude = int(filter_dict['lat_int']) + sign(filter_dict['lat_int'])*(int((filter_dict['lat_dec'])) / (10 ** len(str(filter_dict['lat_dec']))))
                        if (filter_dict["km_int"] == None or filter_dict["km_int"] == 0) and filter_dict["km_dec"] == 0:
                            query = query + \
                                    ("lon=" + str(longitude) +
                                     " AND lat=" + str(latitude))
                        else:
                            R = 6378.1  # Radius of the Earth
                            brng = math.radians(45)
                            d = (int(filter_dict['km_int']) + sign(filter_dict['km_int'])+(int((filter_dict['km_dec'])) / (10 ** len(str(filter_dict['km_dec'])))))/math.sin(brng)
                            lat1 = math.radians(latitude)  # Current lat point converted to radians
                            lon1 = math.radians(longitude)  # Current long point converted to radians
                            lat2 = math.degrees(math.asin(math.sin(lat1) * math.cos(d / R) +
                                                          math.cos(lat1) * math.sin(d / R) * math.cos(brng)))
                            lon2 = math.degrees(lon1 + math.atan2(math.sin(brng) * math.sin(d / R) * math.cos(lat1),
                                                                  math.cos(d / R) - math.sin(lat1) * math.sin(lat2)))
                            brng = math.radians(-135)
                            lat4 = math.degrees(math.asin(math.sin(lat1) * math.cos(d / R) +
                                                          math.cos(lat1) * math.sin(d / R) * math.cos(brng)))
                            lon4 = math.degrees(lon1 + math.atan2(math.sin(brng) * math.sin(d / R) * math.cos(lat1),
                                                                  math.cos(d / R) - math.sin(lat1) * math.sin(
                                                                      lat4)))
                            # Bottom-left and top-right corners of the rectangle
                            query = query + "lat>" + str(lat4) + " AND lat<" + str(lat2) + " AND lon>" + str(lon4) + " AND lon<" + str(lon2)
                    else:
                        query = query[:-4] if ("AND" in query) else query[:-6]
                elif key == "streetName":
                    query = query + "("
                    street = Street.objects.get(name=value)
                    query = query + "(lat>" + str(street.rectangle_lower_lat) + " AND lat<" + str(
                        street.rectangle_upper_lat) + " AND lon>" + str(
                        street.rectangle_lower_lon) + " AND lon<" + str(street.rectangle_upper_lon) + "))"
                elif key == "vehicles":
                    vehicles = json.loads((str(value)).replace("\'", "\""))['vehicles']
                    if vehicles:
                        query = query + "("
                        for vehicle in vehicles:
                            if (vehicle != vehicles[0]):
                                query = query + " OR "
                            query = query + "host=\'" + vehicle + "\'"
                        query = query + ")"
                    else:
                        query = query[:-4] if ("AND" in query) else query[:-6]
    response = list(get_influxdb_client().query(query))
    if (len(response) > 0):
        response = response[0]
        for r in response:
            timestamp = r['time'].partition("T")
            r['date'] = timestamp[0]
            r['time'] = timestamp[2].replace('Z', '')[ 0:8 ]
    geoJsonPointsList = []
    geoJsonPointsList.append("{\"type\": \"FeatureCollection\",\"features\": [")
    for r in response:
        for color in colors:
            if min(color[0], color[1]) < r['LaeqA'] < max(color[0], color[1]):
                geoJsonPointsList.append("{\"type\":\"Feature\",\"properties\":{\"name\":\"LEVEL " + "{:.3f}".format(r['LaeqA']) + "\", \"color\": \"" + colors[color] + "\", \"class\": \"" + r['class'] + "\"},\"geometry\":{\"type\":\"GeometryCollection\",\"geometries\":[{\"type\":\"Point\",\"coordinates\":[" + str(
                    r['lat']) + "," + str(r['lon']) + "]}]}}" + ("" if (r == response[len(response) - 1]) else ","))
                break
    geoJsonPointsList.append("]}")
    geoJsonPointsList = ''.join(geoJsonPointsList)
    max_distance = 0
    centralPoint = [43.270200001993764, -2.9456500000716574]
    for pair in itertools.combinations(response, r=2):
        dlon = abs(math.radians(pair[0]['lon']) - math.radians(pair[1]['lon']))
        dlat = abs(math.radians(pair[0]['lat']) - math.radians(pair[1]['lat']))
        a = math.sin(dlat / 2) ** 2 + math.cos(math.radians(response[1]['lat'])) * math.cos(
            math.radians(pair[0]['lat'])) * math.sin(dlon / 2) ** 2
        distance = 6373.0 * 2 * math.atan2(math.sqrt(a), math.sqrt(1 - a))
        if distance > max_distance:
            max_distance = distance
            x = (math.cos(math.radians(pair[0]['lat'])) * math.cos(math.radians(pair[0]['lon'])) + math.cos(math.radians(pair[1]['lat'])) * math.cos(math.radians(pair[1]['lon'])))/2
            y = (math.cos(math.radians(pair[0]['lat'])) * math.sin(math.radians(pair[0]['lon'])) + math.cos(math.radians(pair[1]['lat'])) * math.sin(math.radians(pair[1]['lon'])))/2
            z = (math.sin(math.radians(pair[0]['lat'])) + math.sin(math.radians(pair[1]['lat'])))/2
            centralPoint[0] = math.atan2(y, x) * 180 / math.pi
            centralPoint[1] = math.atan2(z, math.sqrt(x * x + y * y)) * 180 / math.pi
    zoom = int(max_distance * (-0.002491) + 15.004)
    data = { "points": response, "geoJsonPointsList": geoJsonPointsList, "center":centralPoint, "zoom":zoom}
    return data

def get_noise_map(data):
    geoJsonStreetsList = []
    geoJsonStreetsList.append(
        "{\"type\": \"FeatureCollection\",\"features\": [")
    street_dict = {}
    for point in data["points"]:
        try:
            streets_where_the_point_is = Street.objects.filter(rectangle_upper_lat__gte=point["lat"], rectangle_lower_lat__lte=point["lat"],
                                                               rectangle_upper_lon__gte=point["lon"], rectangle_lower_lon__lte=point["lon"])
            for street in streets_where_the_point_is:
                if street in street_dict:
                    street_dict[street].append(point["LaeqA"])
                else:
                    street_dict[street]=[point["LaeqA"]]
        except Street.DoesNotExist:
            streets_where_the_point_is = []
    for key, value in street_dict.items():
        street_mean = np.array(value).mean()
        baseJson = json.loads(key.geoJson)
        baseJson['properties']['name'] = key.name + " (" + "{:.2f}".format(street_mean) + " dB)"
        for color in colors:
            if min(color[0], color[1]) < street_mean < max(color[0], color[1]):
                baseJson['properties']['color'] = colors[color]
                break
        geoJsonStreetsList.append(str(baseJson).replace("\'", "\""))
        geoJsonStreetsList.append(",")
    if len(geoJsonStreetsList) > 1:
        geoJsonStreetsList = geoJsonStreetsList[:-1]
    geoJsonStreetsList.append("]}")
    geoJsonStreets = ''.join(geoJsonStreetsList)
    return geoJsonStreets

def store_filter(request):
    if request.method == 'POST':
        request.POST = request.POST.copy()
        for element in request.POST:
            if (request.POST[element] == "on"):
                request.POST[element] = True
        vehicles = "{\"vehicles\":["
        if (request.POST.getlist("vehicle_type") and
                request.POST.getlist("vehicleID1") and request.POST.getlist("vehicleID2") and
                request.POST.getlist("vehicleID3") and request.POST.getlist("vehicleID4") and
                len(request.POST.getlist("vehicle_type")) == len(request.POST.getlist("vehicleID1")) and
                len(request.POST.getlist("vehicleID1")) == len(request.POST.getlist("vehicleID2")) and
                len(request.POST.getlist("vehicleID2")) == len(request.POST.getlist("vehicleID3")) and
                len(request.POST.getlist("vehicleID3")) == len(request.POST.getlist("vehicleID4"))):
            for i in range(0, len(request.POST.getlist("vehicle_type"))):
                vehicles = vehicles + "\"" + (str(request.POST.getlist("vehicle_type")[i]) +
                                              str(request.POST.getlist("vehicleID1")[i]) +
                                              str(request.POST.getlist("vehicleID2")[i]) +
                                              str(request.POST.getlist("vehicleID3")[i]) +
                                              str(request.POST.getlist("vehicleID4")[i]) + "\",")
            vehicles = vehicles[:-1]
        vehicles = vehicles + "]}"
        request.POST["vehicles"] = vehicles
        request.POST["filter_date"] = datetime.datetime.now()
        form = FilterModelForm(request.POST)
        if form.is_valid():
            print("Storing...")
            custom_filter = form.save(commit=False)
            custom_filter.user = User.objects.get(username=request.user.username)
            custom_filter.save()
            return JsonResponse({"confirmation":"OK"})
        else:
            data =[]
            errors = json.loads(form.errors.as_json())
            print(errors)
            for error in errors:
                for sub_error in errors[error]:
                    data.append(gettext(sub_error["message"]))
            return JsonResponse({"confirmation":data})


def map(request):
    cities = list(City.objects.values_list('name', flat=True))
    context = {'profile_picture': get_profile_picture(request), 'cities': cities, 'center': [43.270200001993764, -2.9456500000716574], 'zoom':15}
    return render(request, 'zaratamap_app/map.html', context)

def refresh_map(request):
    data = get_filtered_data_from_database(request.POST)
    data["geoJsonStreets"] = get_noise_map(data)
    del data["points"]
    return JsonResponse(data)

def myfilters(request):
    filters_small = User.objects.get(username=request.user.username).filter_set.all().values_list('name', 'filter_date')
    context = {'profile_picture': get_profile_picture(request), 'filters': filters_small}
    return render(request, 'zaratamap_app/myfilters.html', context)

def signup(request):
    data = []
    if request.method == 'POST':
        form = UserCreationForm(request.POST)
        if form.is_valid():
            user = form.save()
            user.groups.add(Group.objects.get(name='citizen'))
            djangologin(request, user)
            return redirect('/home')
        else:
            errors = json.loads(form.errors.as_json())
            print(errors)
            for error in errors:
                for sub_error in errors[error]:
                    data.append(gettext(sub_error["message"]))
    else:
        form = UserCreationForm()
    context = {'profile_picture': get_profile_picture(request), "form": form, "data": data}
    return render(request, 'zaratamap_app/signup.html', context)

def login(request):
    if request.method == 'POST':
        form = AuthenticationForm(data=request.POST)
        if form.is_valid():
            user = form.get_user()
            djangologin(request, user)
            if 'next' in request.POST:
                return redirect(request.POST.get('next'))
            return redirect('index')
        else:
            print(form.errors)
            return render(request, "zaratamap_app/login.html", {"form": form})
    else:
        form = AuthenticationForm()
    return render(request, "zaratamap_app/login.html", {"form": form})

@user_passes_test(lambda u: u.is_superuser)
def createuser(request):
    if request.method == 'POST':
        form = UserCreationFormWithName(request.POST)
        if form.is_valid():
            if (form.cleaned_data["usertype"]) == "IT Administrator":
                user = User.objects.create_superuser(username=form.cleaned_data["username"],
                                                     password=form.cleaned_data["password1"],
                                                     email=form.cleaned_data["email"],
                                                     last_name=form.cleaned_data["last_name"],
                                                     first_name=form.cleaned_data["first_name"])
            else:
                user = form.save()
            djangologin(request, user)
            return redirect('index')
        else:
            print(form.errors)
    else:
        form = UserCreationForm()
    context = {'profile_picture': get_profile_picture(request), "form": form}
    return render(request, 'zaratamap_app/createuser.html', context)

def logout(request):
    if request.method == 'POST':
        djangologout(request)
    return redirect('index')

def profile(request, username):
    error_message_password_change = ""
    error_message_email_change = ""
    error_message_image_change = ""
    if request.method == 'POST':
        if "new_email" in request.POST:
            form = EmailChangeForm(request.POST)
            if form.is_valid():
                user = request.user
                if user.check_password(form.cleaned_data["password_email"]):
                    user.email = form.cleaned_data['new_email']
                    user.save()
                    error_message_email_change = gettext("Your email has been successfully updated.")
                else:
                    error_message_email_change = gettext("Your password was entered incorrectly. Please enter it again.")
            else:
                errors = json.loads(form.errors.as_json())
                print(errors)
                for error in errors:
                    for sub_error in errors[error]:
                        error_message_email_change = gettext(sub_error["message"])
                    break
        elif "new_password1" in request.POST:
            form = PasswordChangeForm(request.user, request.POST)
            if form.is_valid():
                user = form.save()
                update_session_auth_hash(request, user)
                error_message_password_change = gettext("Your password has been successfully updated.")
            else:
                errors = json.loads(form.errors.as_json())
                print(errors)
                for error in errors:
                    for sub_error in errors[error]:
                        error_message_password_change = gettext(sub_error["message"])
                    break
        else:
            form = UploadImageForm(request.POST, request.FILES)
            request_user = User.objects.get(username=request.user.username)
            if form.is_valid():
                try:
                    retrieved = ProfilePicture.objects.get(user=request_user)
                    retrieved.profile_picture = request.FILES['imageToUpload']
                    retrieved.save()
                except ProfilePicture.DoesNotExist:
                    instance = ProfilePicture(user=request_user, profile_picture=request.FILES['imageToUpload'])
                    instance.save()
            else:
                errors = json.loads(form.errors.as_json())
                for error in errors:
                    for sub_error in errors[error]:
                        error_message_image_change = gettext("Please, upload a valid image.")
                    break
    context = {'profile_picture': get_profile_picture(request), 'error_message_password_change':error_message_password_change, 'error_message_email_change':error_message_email_change, 'error_message_image_change': error_message_image_change}
    return render(request, 'zaratamap_app/profile.html', context)

def get_profile_picture(request):
    try:
        profile_picture = settings.MEDIA_URL + User.objects.get(
            username=request.user.username).profilepicture.profile_picture.name
    except User.DoesNotExist:
        profile_picture = settings.MEDIA_URL + "profile_pictures\profileDog.jpg"
    except User.profilepicture.RelatedObjectDoesNotExist:
        profile_picture = settings.MEDIA_URL + "profile_pictures\profileDog.jpg"
    return profile_picture

def octaves(request):
    context = {'profile_picture': get_profile_picture(request)}
    return render(request, 'zaratamap_app/octaves.html', context)

def retrieve_octaves(request):
    filters = [[[0.2343, 0.4686, 0.2343, 1, -1.8939, 0.89516], [1, -2.0001, 1.0001, 1, -1.9946, 0.99462],
                [1, -1.9999, 0.99986, 1, -0.22456, 0.012607]],
               [[0.0028997, -4.6561e-10, -0.0028997, 1, -1.994, 0.99403],
                [0.0028997, 0.0057993, 0.0028996, 1, -1.9961, 0.99623],
                [0.0028997, -0.0057993, 0.0028996, 1, -1.9981, 0.99814]],
               [[0.0057424, -9.2208e-10, -0.0057424, 1, -1.9883, 0.98854],
                [0.0057424, 0.011485, 0.0057423, 1, -1.9921, 0.99256],
                [0.0057424, -0.011485, 0.0057423, 1, -1.9958, 0.99594]],
               [[0.011441, -1.8372e-09, -0.011441, 1, -1.9761, 0.9772],
                [0.011441, 0.022882, 0.011441, 1, -1.9832, 0.98518], [0.011441, -0.022882, 0.011441, 1, -1.9913, 0.99191]],
               [[0.02271, -3.6467e-09, -0.02271, 1, -1.9507, 0.95492], [0.02271, 0.04542, 0.02271, 1, -1.9628, 0.97058],
                [0.02271, -0.04542, 0.02271, 1, -1.9816, 0.98389]],
               [[0.04475, -7.1858e-09, -0.044751, 1, -1.8954, 0.91178],
                [0.04475, 0.089501, 0.04475, 1, -1.9116, 0.94211], [0.04475, -0.089501, 0.04475, 1, -1.9588, 0.96799]],
               [[0.08697, -1.3965e-08, -0.086971, 1, -1.7681, 0.83067],
                [0.08697, 0.17394, 0.086969, 1, -1.7703, 0.88818], [0.08697, -0.17394, 0.086969, 1, -1.9008, 0.93674]],
               [[0.16487, -2.6475e-08, -0.16488, 1, -1.4571, 0.68551], [0.16487, 0.32975, 0.16487, 1, -1.3581, 0.79336],
                [0.16487, -0.32975, 0.16487, 1, -1.7372, 0.87536]],
               [[0.30049, -4.8251e-08, -0.30049, 1, -0.68723, 0.44235],
                [0.30049, 0.60098, 0.30049, 1, -0.22883, 0.66257], [0.30049, -0.60098, 0.30049, 1, -1.2517, 0.74954]],
               [[0.524, -8.4141e-08, -0.52401, 1, 0.85657, 0.047587], [0.524, -1.048, 0.524, 1, -0.076869, 0.39503],
                [0.524, 1.048, 0.524, 1, 1.8159, 0.8489]]]
    filter_responses = []
    for filter in filters:
        w, h = signal.sosfreqz(filter, worN=2000)
        filter_responses.append((20 * np.log10(np.maximum(np.abs(h), 1e-5))).tolist())
    x = list(range(0, 20000, 10))
    return JsonResponse({"x": x, "AWeighting": filter_responses[0], "octave63": filter_responses[1], "octave125": filter_responses[2], "octave250": filter_responses[3], "octave500": filter_responses[4], "octave1000": filter_responses[5], "octave2000": filter_responses[6], "octave4000": filter_responses[7], "octave8000": filter_responses[8], "octave16000": filter_responses[9]})

def delete_filter(request):
    try:
        User.objects.get(username=request.user.username).filter_set.get(name=request.POST["name"]).delete()
        return JsonResponse({"confirmation":"OK"})
    except:
        print("Error deleting filter")
        return JsonResponse({"confirmation":"ERROR"})

def update_filter(request):
    try:
        User.objects.get(username=request.user.username).filter_set.get(name=request.POST["name"]).delete()
        store_v = store_filter(request)
        if json.loads(store_v.content)["confirmation"] == "OK":
            return JsonResponse({"confirmation":gettext("Filter successfully updated.")})
        else:
            return JsonResponse({"confirmation": gettext("Something went wrong when updating filter. The filter was deleted from the database.")})
    except:
        print("Error updating filter")
        return JsonResponse({"confirmation": gettext("Filter could not be updated, please try again.")})

def the_project(request):
    context = {'profile_picture': get_profile_picture(request)}
    return render(request, 'zaratamap_app/the_project.html', context)

def contact_us(request):
    context = {'profile_picture': get_profile_picture(request)}
    return render(request, 'zaratamap_app/contact_us.html', context)

def consequences(request):
    context = {'profile_picture': get_profile_picture(request)}
    return render(request, 'zaratamap_app/consequences.html', context)

def regulation(request):
    context = {'profile_picture': get_profile_picture(request)}
    return render(request, 'zaratamap_app/regulation.html', context)