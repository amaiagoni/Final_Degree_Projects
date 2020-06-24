from django import forms
from django.contrib.auth.forms import UserCreationForm
from django.contrib.auth.models import User, Group
from django.utils.safestring import mark_safe

class UserCreationForm(UserCreationForm):
    email = forms.EmailField(required=True, label='Email')

    class Meta:
        model = User
        fields = ("username", "email", "password1", "password2")

    def save(self, commit=True):
        user = super(UserCreationForm, self).save(commit=False)
        user.email = self.cleaned_data["email"]
        if commit:
            user.save()
        return user

class UserCreationFormWithName(UserCreationForm):
    email = forms.EmailField(required=True, label='Email')
    first_name = forms.CharField(required=True)
    last_name = forms.CharField(required=True)
    usertype = forms.CharField(widget=forms.RadioSelect())

    class Meta:
        model = User
        fields = ("username", "email", "password1", "password2")

    def save(self, commit=True):
        user = super(UserCreationForm, self).save(commit=False)
        user.email = self.cleaned_data["email"]
        user.first_name = self.cleaned_data["first_name"]
        user.last_name = self.cleaned_data["last_name"]
        user.groups.add(Group.objects.get(name='zaratamap_admin'))
        if commit:
            user.save()
        return user

class FilterForm(forms.Form):
    date_from = forms.DateField(label=mark_safe('date_from'), required=False)
    date_to = forms.DateField(label=mark_safe('date_to'), required=False)
    time_from = forms.TimeField(label=mark_safe('time_from'), required=False)
    time_to = forms.TimeField(label=mark_safe('time_to'), required=False)
    noise_from = forms.FloatField(label=mark_safe('noise_from'), required=False)
    noise_to = forms.FloatField(label=mark_safe('noise_to'), required=False)
    above_max_night = forms.BooleanField(label=mark_safe('above_max_night'), required=False)
    above_max_day = forms.BooleanField(label=mark_safe('above_max_day'), required=False)
    streetName = forms.CharField(label=mark_safe('streetName'), required=False)
    lon_int = forms.DecimalField(label=mark_safe('lon_int'), required=False)
    lon_dec = forms.DecimalField(label=mark_safe('lon_dec'), required=False)
    lat_int = forms.DecimalField(label=mark_safe('lat_int'), required=False)
    lat_dec = forms.DecimalField(label=mark_safe('lat_dec'), required=False)
    km_int = forms.DecimalField(label=mark_safe('km_int'), required=False)
    km_dec = forms.DecimalField(label=mark_safe('km_dec'), required=False)

class EmailChangeForm(forms.Form):
    new_email = forms.EmailField(required=True, label=mark_safe('new_email'))
    password_email = forms.CharField(widget=forms.PasswordInput, required=True, label=mark_safe('password_email'))

class UploadImageForm(forms.Form):
    imageToUpload = forms.ImageField(required=True, label=mark_safe('imageToUpload'))