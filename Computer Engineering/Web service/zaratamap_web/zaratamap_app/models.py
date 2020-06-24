from django.db import models
from django.forms import ModelForm
from django import forms
from jsonfield import JSONField
from django.utils.safestring import mark_safe
from django.conf import settings
from django.contrib.auth.models import User
from django.utils import timezone

class VehicleFilter(models.Model):
    ...
    class Meta:
        ...
        permissions = (("can_view", "View filters related with vehicle in data-table.html"),)

class City(models.Model):
    name = models.TextField()
    rectangle_borders = JSONField()

    rectangle_upper_lat = models.FloatField()
    rectangle_upper_lon = models.FloatField()
    rectangle_lower_lat = models.FloatField()
    rectangle_lower_lon = models.FloatField()

    def __str__(self):
        return self.name

class Street(models.Model):
    city = models.ForeignKey(City, on_delete=models.CASCADE)
    name = models.TextField()
    geoJson = models.TextField()
    rectangle_borders = JSONField()
    rectangle_upper_lat = models.FloatField()
    rectangle_upper_lon = models.FloatField()
    rectangle_lower_lat = models.FloatField()
    rectangle_lower_lon = models.FloatField()
    def __str__(self):
        return self.name

class Filter(models.Model):
    user = models.ForeignKey(User, on_delete=models.CASCADE)
    name = models.TextField(unique=True)
    filter_date = models.DateField(default=timezone.now)
    date_from = models.DateField(blank=True, null=True)
    date_to = models.DateField(blank=True, null=True)
    time_from = models.TimeField(blank=True, null=True)
    time_to = models.TimeField(blank=True, null=True)
    noise_from = models.FloatField(blank=True, null=True)
    noise_to = models.FloatField(blank=True, null=True)
    above_max_night = models.BooleanField(blank=True, null=True)
    above_max_day = models.BooleanField(blank=True, null=True)
    streetName = models.TextField(blank=True, null=True)
    lon_int = models.DecimalField(max_digits=3, decimal_places=0, blank=True, null=True)
    lon_dec = models.DecimalField(max_digits=5, decimal_places=0, blank=True, null=True)
    lat_int = models.DecimalField(max_digits=3, decimal_places=0, blank=True, null=True)
    lat_dec = models.DecimalField(max_digits=5, decimal_places=0, blank=True, null=True)
    km_int = models.DecimalField(max_digits=3, decimal_places=0, blank=True, null=True)
    km_dec = models.DecimalField(max_digits=5, decimal_places=0, blank=True, null=True)
    vehicles = JSONField()

    def __str__(self):
        return self.name

class FilterModelForm(ModelForm):
    class Meta:
        model = Filter
        exclude=('user',)
            
class ProfilePicture(models.Model):
    user = models.OneToOneField(User, on_delete=models.CASCADE)
    profile_picture = models.ImageField(default="media\profile_pictures\profileDog.jpg", upload_to="profile_pictures/")