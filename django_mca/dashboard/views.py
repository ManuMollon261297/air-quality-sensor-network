from django.shortcuts import render
from django.views import generic
from django.core.paginator import Paginator, EmptyPage, PageNotAnInteger
from django.contrib.auth.decorators import login_required
from django.contrib.auth.mixins import LoginRequiredMixin
from .models import Node, DataLog
from django.contrib.auth.models import User
import plotly
import plotly.express as px
import numpy as np
from math import floor

# Create your views here.

def index(request):
    """View function for home page of site."""

    # Number of visits to this view, as counted in the session variable.
    num_visits = request.session.get('num_visits', 0)
    request.session['num_visits'] = num_visits + 1

    # Generate counts of some of the main objects
    num_nodes = Node.objects.all().count()
    num_users = User.objects.all().count()
    num_logs = DataLog.objects.all().count()

    context = {
        'num_nodes': num_nodes,
        'num_users': num_users,
        'num_logs': num_logs,
        'num_visits': num_visits,
    }

    # Render the HTML template index.html with the data in the context variable
    return render(request, 'index.html', context=context)
    
def percentageToHsl(percentage, hue0, hue1):
    hue = (percentage * (hue1 - hue0)) + hue0
    return 'hsl(' + str(hue) + ', 100%, 60%)'

class NodeListView(LoginRequiredMixin, generic.ListView):
    model = Node
    paginate_by = 10

@login_required
def NodeDetailView(request, pk):
    """View function for specific node."""

    node = Node.objects.get(node_id=pk)
    node_node_id = node.node_id
    node_user_id = node.user_id
    node_location = node.location
    node_created_at = node.created_at
    node_updated_at = node.updated_at
    paginator = Paginator(DataLog.objects.filter(node_id=pk).order_by('-measured_at'), 2) # Show 5 contacts per page
    page = request.GET.get('page')
    try:
        node_measurements = paginator.page(page)
    except PageNotAnInteger:
        # If page is not an integer, deliver first page.
        node_measurements = paginator.page(1)
    except EmptyPage:
        # If page is out of range (e.g. 9999), deliver last page of results.
        node_measurements = paginator.page(paginator.num_pages)

    logs = DataLog.objects.filter(node_id=pk).order_by('-measured_at')
    if len(logs) > 0:
        last_measurement = logs[0]
        current_battery_level = last_measurement.v_value
        conc_i = floor(last_measurement.pm_value)
    else:
        current_battery_level = 0.0
        conc_i = 0
 
    AQI_h_arr = [50, 100, 150, 200, 300, 500]
    AQI_lo_arr = [0, 51, 101, 151, 201, 301]
    conc_hi_arr = [54, 154, 254, 354, 424, 604]
    conc_lo_arr = [0, 55, 155, 255, 355, 425]
    AQI_color_arr = ['rgb(57, 233, 57)','rgb(230, 233, 57)','rgb(233, 163, 57)','rgb(233, 57, 57)','rgb(163, 57, 233)','gb(110, 27, 48)']
    AQI_text_arr = ['Good','Moderate','Unhealthy for Sensitive Groups','Unhealthy','Very Unhealthy','Hazardous']

    diff_arr = np.asarray(conc_lo_arr) - conc_i
    abs_arr = np.abs(np.asarray(conc_lo_arr) - conc_i)
    smallest_diff_index = abs_arr.argmin()
    closest_element = conc_lo_arr[smallest_diff_index]
    if conc_i-closest_element < 0:
        smallest_diff_index = smallest_diff_index-1
        
    AQI_h = AQI_h_arr[smallest_diff_index]
    AQI_lo = AQI_lo_arr[smallest_diff_index]
    conc_hi = conc_hi_arr[smallest_diff_index]
    conc_lo = conc_lo_arr[smallest_diff_index]
    current_aqi = ((AQI_h-AQI_lo)/(conc_hi-conc_lo))*(conc_i-conc_lo)+AQI_lo
    current_aqi = round(current_aqi)
    AQI_color = AQI_color_arr[smallest_diff_index]
    AQI_text = AQI_text_arr[smallest_diff_index]

    percentage = 1 - (current_battery_level/100)
    battery_color = percentageToHsl(percentage, 120, 0)
    
    context = {
        'node': node,
        'node_id': node_node_id,
        'node_user_id': node_user_id,
        'node_location': node_location,
        'node_created_at': node_created_at,
        'node_updated_at': node_updated_at,
        'current_aqi': current_aqi,
        'current_battery_level': current_battery_level,
        'page_obj': node_measurements,
        'AQI_color': AQI_color,
        'AQI_text': AQI_text,
        'battery_color': battery_color,
    }

    # Render the HTML template index.html with the data in the context variable
    return render(request, 'node_detail.html', context=context)

class NodesByUserListView(LoginRequiredMixin, generic.ListView):
    """Generic class-based view listing nodes of the current user."""
    model = Node
    template_name ='dashboard/nodes_by_user.html'
    paginate_by = 10

    def get_queryset(self):
        return Node.objects.filter(user_id=self.request.user)

@login_required
def NodeGraphsView(request, pk):
    """View function for specific node."""
    measurements = DataLog.objects.filter(node_id=pk)
    time = []
    pm = []
    t = []
    h = []
    p = []
    v = []
    for measurement in  measurements:
        time.append(measurement.measured_at)
        pm.append(measurement.pm_value)
        t.append(measurement.t_value)
        h.append(measurement.h_value)
        p.append(measurement.p_value)
        v.append(measurement.v_value)


    # fig is plotly figure object and graph_div the html code for displaying the graph
    fig = px.line(x=time, y=pm, title="PM 10")
    fig.update_layout(xaxis_title="Time", yaxis_title="µg/m3", paper_bgcolor='rgba(0,0,0,0)')
    pm_graph_div = plotly.offline.plot(fig, auto_open = False, output_type="div")

    fig = px.line(x=time, y=t, title="Temperature")
    fig.update_layout(xaxis_title="Time", yaxis_title="°C", paper_bgcolor='rgba(0,0,0,0)')
    t_graph_div = plotly.offline.plot(fig, auto_open = False, output_type="div")

    fig = px.line(x=time, y=h, title="Humidity")
    fig.update_layout(xaxis_title="Time", yaxis_title="RH [%]", paper_bgcolor='rgba(0,0,0,0)')
    h_graph_div = plotly.offline.plot(fig, auto_open = False, output_type="div")

    fig = px.line(x=time, y=p, title="Pressure")
    fig.update_layout(xaxis_title="Time", yaxis_title="hPa", paper_bgcolor='rgba(0,0,0,0)')
    p_graph_div = plotly.offline.plot(fig, auto_open = False, output_type="div")

    fig = px.line(x=time, y=v, title="Battery Level")
    fig.update_layout(xaxis_title="Time", yaxis_title="Percentage [%]", paper_bgcolor='rgba(0,0,0,0)')
    v_graph_div = plotly.offline.plot(fig, auto_open = False, output_type="div")

    context = {
        'node_pk': pk,
        'pm_graph_div': pm_graph_div,
        't_graph_div': t_graph_div,
        'h_graph_div': h_graph_div,
        'p_graph_div': p_graph_div,
        'v_graph_div': v_graph_div,
    }

    # Render the HTML template index.html with the data in the context variable
    return render(request, 'node_graph.html', context=context)
