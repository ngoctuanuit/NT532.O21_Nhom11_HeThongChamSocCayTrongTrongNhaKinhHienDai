import json
import boto3
from datetime import datetime
import time

s3 = boto3.client('s3')
BUCKET_NAME = 'uitiotprocess'

def lambda_handler(event, context):
    try:
        # Xử lý payload đầu vào
        payload = event.get('payload', event)
        if isinstance(payload, str):
            payload = json.loads(payload)
        
        # Tạo timestamp ISO 8601 với microsecond và không timezone
        current_time = datetime.utcnow()
        timestamp_str = current_time.isoformat(timespec='microseconds')
        
        # Tạo tên file theo định dạng yêu cầu
        filename = f"sensor-data/{timestamp_str}.json"
        
        # Thêm metadata vào payload nếu cần
        enriched_payload = {
            **payload,
            "processing_timestamp": timestamp_str,
            "file_size": len(json.dumps(payload).encode('utf-8'))  # Tính trước kích thước file
        }
        
        # Upload lên S3
        s3.put_object(
            Bucket=BUCKET_NAME,
            Key=filename,
            Body=json.dumps(enriched_payload, indent=2),
            ContentType='application/json',
            Metadata={
                'processed-by': 'face-recognition-lambda',
                'source-device': payload.get('device_id', 'unknown')
            }
        )
        
        return {
            'statusCode': 200,
            'body': {
                'message': 'File uploaded successfully',
                's3_path': f"s3://{BUCKET_NAME}/{filename}",
                'timestamp': timestamp_str
            }
        }
        
    except Exception as e:
        return {
            'statusCode': 500,
            'body': f"Error: {str(e)}"
        }

# import json
# import boto3
# import base64
# from datetime import datetime

# s3 = boto3.client('s3')
# BUCKET_NAME = 'uitiotprocess'

# def lambda_handler(event, context):
#     try:
#         print("Received event:", json.dumps(event))  # ✅ Log để debug

#         # Xử lý payload base64 nếu đến từ IoT Core
#         if 'payload' in event:
#             # Trường hợp dùng IoT Rule Action với 'payload'
#             payload_raw = event['payload']
#             if isinstance(payload_raw, str):
#                 # Nếu là base64 string (chuẩn IoT Core)
#                 payload_decoded = base64.b64decode(payload_raw).decode('utf-8')
#                 payload = json.loads(payload_decoded)
#             else:
#                 # Nếu payload không phải base64 thì assume là dict
#                 payload = payload_raw
#         else:
#             # Nếu event trực tiếp là JSON (test API Gateway hoặc local)
#             payload = event if isinstance(event, dict) else json.loads(event)

#         # Tạo timestamp ISO 8601, không có colon để làm tên file hợp lệ
#         current_time = datetime.utcnow()
#         timestamp_str = current_time.isoformat(timespec='microseconds').replace(':', '-')

#         # Tạo đường dẫn lưu file
#         filename = f"sensor-data/{timestamp_str}.json"

#         # Enrich thêm metadata
#         enriched_payload = {
#             **payload,
#             "processing_timestamp": current_time.isoformat(timespec='seconds') + 'Z',
#             "file_size": len(json.dumps(payload).encode('utf-8'))
#         }

#         # Ghi vào S3
#         s3.put_object(
#             Bucket=BUCKET_NAME,
#             Key=filename,
#             Body=json.dumps(enriched_payload, indent=2),
#             ContentType='application/json',
#             Metadata={
#                 'processed-by': 'iot-data-lambda',
#                 'source-device': str(payload.get('device_id', 'unknown'))
#             }
#         )

#         return {
#             'statusCode': 200,
#             'body': {
#                 'message': 'File uploaded successfully',
#                 's3_path': f"s3://{BUCKET_NAME}/{filename}",
#                 'timestamp': timestamp_str
#             }
#         }

#     except Exception as e:
#         print("Error:", str(e))  # ✅ In lỗi ra CloudWatch
#         return {
#             'statusCode': 500,
#             'body': {
#                 'error': str(e)
#             }
#         }
