from airflow import DAG
from airflow.operators.python import PythonOperator, BranchPythonOperator
from airflow.operators.bash import BashOperator

import sqlite3
import requests
import datetime
import os

def connect(filename='./db.sqlite3'):
    try:
        sqliteConnection = sqlite3.connect(filename)
        cursor = sqliteConnection.cursor()
        print("Database Opened and Successfully Connected to SQLite")

        sqlite_select_Query = "select sqlite_version();"
        cursor.execute(sqlite_select_Query)
        record = cursor.fetchall()
        print("SQLite Database Version is: ", record)
        return cursor, sqliteConnection
    except sqlite3.Error as error:
        print("Error while connecting to sqlite", error)
        return None, None

def insert(cursor, sqliteConnection, query):
    try:
        sqlite_insert_query = query
        cursor.execute(sqlite_insert_query)
        sqliteConnection.commit()
        print("Record inserted successfully into table ", cursor.rowcount)
        return True
    except sqlite3.Error as error:
        print("Failed to insert data into table", error)
        return False

def add_measurment(measured_at, pm_value, t_value, h_value, p_value, v_value, node_id_id):        
    query =     f"""
                INSERT INTO dashboard_datalog
                (measured_at, pm_value, t_value, h_value, p_value, v_value, node_id_id) 
                VALUES 
                ('{measured_at}', {pm_value}, {t_value}, {h_value}, {p_value}, {v_value}, {node_id_id})
                """

    cursor, sqliteConnection = connect()
    if sqliteConnection:
        insert(cursor, sqliteConnection, query)
        cursor.close()
        sqliteConnection.close()
        print("The SQLite connection is closed")

# Date format: YYYY-MM-DD%20HH:NN:SS.
def dag_run(**context,): # start_date, end_date

    start_date = (context['data_interval_start']-datetime.timedelta(hours=3, minutes=0)).strftime('20%y-%m-%d %H:%M:%S').replace(' ', '%20', 1)
    end_date = (context['data_interval_end']-datetime.timedelta(hours=3, minutes=0)).strftime('20%y-%m-%d %H:%M:%S').replace(' ', '%20', 1)
    URL = f"https://api.thingspeak.com/channels/1493992/feeds.json?api_key=ZUEQ00Z73HPP01L3&timezone=America/Argentina/Buenos_Aires&start={start_date}&end={end_date}"
    r = requests.get(url = URL)
    data = r.json()
    info = data['channel']
    results = data['feeds']

    print('Start date:  ', start_date)
    print('End date:    ', end_date)

    for result in results:
        print('Result:  ', result)
        datetime_obj = datetime.datetime.fromisoformat(result['created_at'].rstrip('Z'))
        add_measurment( str(datetime_obj)
                        , float(result['field1'])
                        , float(result['field2'])
                        , float(result['field3'])
                        , float(result['field4'])
                        , float(result['field5'])
                        , int(result['field6']))

with DAG("thingspeak_to_db_dag", start_date=datetime.datetime(2022, 1, 31), schedule_interval='*/5 * * * *', catchup=False) as dag:
    dag_run = PythonOperator(
        task_id="dag_run"
        , python_callable=dag_run
        , provide_context=True
    )
    dag_run