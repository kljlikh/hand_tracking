import cv2
import mediapipe as mp
import requests
import time

# ===== IP ESP32 =====
ESP32_IP = "http://192.168.93.119"   # ← thay bằng IP thực tế từ Serial Monitor

# ===== MediaPipe =====
mp_hands = mp.solutions.hands
mp_draw = mp.solutions.drawing_utils

hands = mp_hands.Hands(
    static_image_mode=False,
    max_num_hands=1,
    min_detection_confidence=0.7,
    min_tracking_confidence=0.7
)

# ===== Camera =====
cap = cv2.VideoCapture(0)

# ===== Chống spam lệnh =====
last_command = ""
last_time = 0

def send_command(cmd):
    global last_command, last_time

    if cmd == last_command and time.time() - last_time < 0.25:  # 250ms
        return

    try:
        requests.get(f"{ESP32_IP}/?cmd={cmd}", timeout=0.3)
        last_command = cmd
        last_time = time.time()
        print(f"Sent: {cmd}")
    except:
        pass  # im lặng nếu lỗi mạng

# ===== Đếm ngón tay =====
def count_fingers(hand_landmarks):
    tips = [4, 8, 12, 16, 20]
    fingers = []

    # Ngón cái (dùng trục X)
    if hand_landmarks.landmark[4].x < hand_landmarks.landmark[3].x:
        fingers.append(1)
    else:
        fingers.append(0)

    # 4 ngón còn lại (trục Y)
    for tip in tips[1:]:
        if hand_landmarks.landmark[tip].y < hand_landmarks.landmark[tip - 2].y:
            fingers.append(1)
        else:
            fingers.append(0)

    return sum(fingers)

# ===== MAIN LOOP =====
while True:
    ret, frame = cap.read()
    if not ret:
        break

    frame = cv2.flip(frame, 1)
    h, w, _ = frame.shape

    rgb = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
    result = hands.process(rgb)

    command = "STOP"

    if result.multi_hand_landmarks:
        for hand in result.multi_hand_landmarks:
            mp_draw.draw_landmarks(frame, hand, mp_hands.HAND_CONNECTIONS)
            lm = hand.landmark

            fingers = count_fingers(hand)

            # Vị trí ngón trỏ
            x = int(lm[8].x * w)
            y = int(lm[8].y * h)
            cv2.circle(frame, (x, y), 12, (0, 255, 0), -1)

            # Logic điều khiển
            if fingers >= 4 or fingers == 0:   # mở bàn tay hoặc nắm tay = STOP
                command = "STOP"
            else:
                if x < w * 0.25:
                    command = "LEFT"
                elif x > w * 0.75:
                    command = "RIGHT"
                elif y < h * 0.25:
                    command = "FORWARD"
                elif y > h * 0.75:
                    command = "BACKWARD"
                else:
                    command = "STOP"

            send_command(command)

    # Vẽ vùng điều khiển
    cv2.rectangle(frame, (0, 0), (int(w*0.25), h), (255, 0, 0), 2)      # LEFT
    cv2.rectangle(frame, (int(w*0.75), 0), (w, h), (255, 0, 0), 2)      # RIGHT
    cv2.rectangle(frame, (0, 0), (w, int(h*0.25)), (255, 0, 0), 2)      # FORWARD
    cv2.rectangle(frame, (0, int(h*0.75)), (w, h), (255, 0, 0), 2)      # BACKWARD

    cv2.putText(frame, f"CMD: {command} | Fingers: {fingers if 'fingers' in locals() else 0}", 
                (10, 50), cv2.FONT_HERSHEY_SIMPLEX, 1, (0, 255, 255), 2)

    cv2.imshow("Gesture Control - Press ESC to exit", frame)

    if cv2.waitKey(1) & 0xFF == 27:   # ESC
        break

# Clean up
cap.release()
cv2.destroyAllWindows()