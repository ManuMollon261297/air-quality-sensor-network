from django.db import models
from django.urls import reverse
from django.contrib.auth.models import User

# Create your models here.

class Node(models.Model):

    # Fields
    node_id = models.IntegerField(primary_key=True, unique=False)
    user_id = models.ForeignKey(User, on_delete=models.CASCADE, unique=False)
    location = models.CharField(max_length=50, help_text='Location of the node')
    created_at = models.DateTimeField(auto_now=False, auto_now_add=False)
    updated_at = models.DateTimeField(auto_now=False, auto_now_add=False)

    # Metadata
    class Meta:
        ordering = ['user_id','node_id']

    # Methods
    def get_absolute_url(self):
        """Returns the url to access a particular instance of Node."""
        return reverse('node-detail', args=[str(self.node_id)])

    def __str__(self):
        """String for representing the Node object (in Admin site etc.)."""
        return str(self.node_id)

class DataLog(models.Model):

    # Fields
    measured_at = models.DateTimeField(auto_now=False, auto_now_add=False)
    node_id = models.ForeignKey(Node, on_delete=models.CASCADE, unique=False)
    pm_value = models.FloatField()
    t_value = models.FloatField()
    h_value = models.FloatField()
    p_value = models.FloatField()
    v_value = models.FloatField()

    # Metadata
    class Meta:
        ordering = ['node_id','measured_at']

    # Methods
    def get_absolute_url(self):
        """Returns the url to access a particular instance of MyModelName."""
        return reverse('log-detail', args=[str(self.id)])

    def __str__(self):
        """String for representing the DataLog object (in Admin site etc.)."""
        return str(self.measured_at) + '-' + str(self.node_id)
