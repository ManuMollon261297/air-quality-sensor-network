from django.urls import include, path
from . import views

urlpatterns = [
    path('', views.index, name='index'),
    path('nodes/', views.NodeListView.as_view(), name='nodes'),
    path('node/<int:pk>', views.NodeDetailView, name='node-detail'),
    path('mynodes/', views.NodesByUserListView.as_view(), name='my-nodes'),
    path('node/<int:pk>/graphs', views.NodeGraphsView, name='node-graphs'),
]