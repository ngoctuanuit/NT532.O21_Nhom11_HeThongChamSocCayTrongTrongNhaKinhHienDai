import boto3
import time
import uuid
import datetime

def lambda_handler(event, context):
    s3 = boto3.client('s3')
    bucket = 'uitiotprocess'  
    prefix = 'sensor-node/'   

    # Step 1: Wait 30 seconds
    time.sleep(5)

    # Step 2: List objects in the prefix
    response = s3.list_objects_v2(Bucket=bucket, Prefix=prefix)
    if 'Contents' not in response or len(response['Contents']) < 2:
        return {
            'statusCode': 400,
            'body': 'Not enough files in sensor-node to process'
        }

    # Step 3: Sort files by LastModified descending (newest first)
    sorted_files = sorted(response['Contents'], key=lambda x: x['LastModified'], reverse=True)

    # Step 4: Pick the second newest file (kề cuối)
    target_file = sorted_files[1]['Key']

    # Step 5: Prepare SageMaker client and job name
    sagemaker = boto3.client('sagemaker')
    job_name = f'process-sensor-data-{datetime.datetime.now().strftime("%Y%m%d-%H%M%S")}-{uuid.uuid4().hex[:8]}'

    # Step 6: Trigger SageMaker processing job
    response = sagemaker.create_processing_job(
        ProcessingJobName=job_name,
        RoleArn='arn:aws:iam::526718635506:role/SageMakerLambdaTriggerRole',
        AppSpecification={
            'ImageUri': '683313688378.dkr.ecr.ap-southeast-1.amazonaws.com/sagemaker-scikit-learn:1.2-1-cpu-py3',
            'ContainerEntrypoint': ['python3', '/opt/ml/processing/input/code/process_script.py']
        },
        ProcessingInputs=[
            {
                'InputName': 'sensor-data',
                'S3Input': {
                    'S3Uri': f's3://{bucket}/{target_file}',
                    'LocalPath': '/opt/ml/processing/input/data/',
                    'S3DataType': 'S3Prefix',
                    'S3InputMode': 'File'
                }
            },
            {
                'InputName': 'code',
                'S3Input': {
                    'S3Uri': 's3://uitiotprocess/scripts/',
                    'LocalPath': '/opt/ml/processing/input/code/',
                    'S3DataType': 'S3Prefix',
                    'S3InputMode': 'File'
                }
            }
        ],
        ProcessingOutputConfig={
            'Outputs': [
                {
                    'OutputName': 'processed-data',
                    'S3Output': {
                        'S3Uri': 's3://uitiotprocess/processed-data/',
                        'LocalPath': '/opt/ml/processing/output/',
                        'S3UploadMode': 'EndOfJob'
                    }
                }
            ]
        },
        ProcessingResources={
            'ClusterConfig': {
                'InstanceCount': 1,
                'InstanceType': 'ml.t3.medium',
                'VolumeSizeInGB': 10
            }
        }
    )

    return {
        'statusCode': 200,
        'body': f'SageMaker processing job started on file: {target_file}, job name: {job_name}'
    }
