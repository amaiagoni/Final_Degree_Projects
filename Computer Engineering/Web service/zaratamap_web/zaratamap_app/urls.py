from django.urls import path
from . import views

urlpatterns = [
    path('home', views.index, name='index'),
    path('table', views.table, name='table'),
    path('table/<str:filter>/', views.table, name='table'),
    path('map', views.map, name='map'),
    path('login', views.login, name='login'),
    path('signup', views.signup, name='signup'),
    path('create-user', views.createuser, name='createuser'),
    path('logout', views.logout, name='logout'),
    path('table_row', views.table_row, name='table_row'),
    path('store_filter', views.store_filter, name='store_filter'),
    path('myfilters', views.myfilters, name='myfilters'),
    path('myprofile/<str:username>/', views.profile, name='profile'),
    path('refresh_map', views.refresh_map, name='refresh_map'),
    path('octaves', views.octaves, name='octaves'),
    path('retrieve_octaves', views.retrieve_octaves, name='retrieve_octaves'),
    path('delete_filter', views.delete_filter, name='delete_filter'),
    path('update_filter', views.update_filter, name='update_filter'),
    path('the_project', views.the_project, name='the_project'),
    path('contact_us', views.contact_us, name='contact_us'),
    path('consequences', views.consequences, name='consequences'),
    path('regulation', views.regulation, name='regulation')
]