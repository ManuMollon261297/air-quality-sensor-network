from django.contrib import admin

# Register your models here.

from .models import DataLog, Node

admin.site.register(DataLog)
admin.site.register(Node)