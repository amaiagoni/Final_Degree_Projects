# Generated by Django 2.2.6 on 2020-05-13 19:50

from django.db import migrations, models


class Migration(migrations.Migration):

    dependencies = [
        ('zaratamap_app', '0003_street_rectangle_borders'),
    ]

    operations = [
        migrations.CreateModel(
            name='Filter',
            fields=[
                ('id', models.AutoField(auto_created=True, primary_key=True, serialize=False, verbose_name='ID')),
                ('name', models.TextField()),
                ('date_from', models.DateField()),
                ('date_to', models.DateField()),
                ('time_from', models.TimeField()),
                ('time_to', models.TimeField()),
                ('noise_from', models.FloatField()),
                ('noise_to', models.FloatField()),
                ('above_max_night', models.BooleanField()),
                ('above_max_day', models.BooleanField()),
                ('streetName', models.TextField()),
                ('lon_int', models.DecimalField(decimal_places=1000, max_digits=1000)),
                ('lon_dec', models.DecimalField(decimal_places=1000, max_digits=1000)),
                ('lat_int', models.DecimalField(decimal_places=1000, max_digits=1000)),
                ('lat_dec', models.DecimalField(decimal_places=1000, max_digits=1000)),
                ('km_int', models.DecimalField(decimal_places=1000, max_digits=1000)),
                ('km_dec', models.DecimalField(decimal_places=1000, max_digits=1000)),
            ],
        ),
    ]