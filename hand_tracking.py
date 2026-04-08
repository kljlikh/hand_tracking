import cv2
import mediapipe as mp
import requests
import time

# ===== IP ESP32 =====
ESP32_IP = "http://192.168.93.113"

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

    if cmd == last_command and time.time() - last_time < 0.3:
        return

    try:
        requests.get(f"{ESP32_IP}/?cmd={cmd}", timeout=0.2)
        last_command = cmd
        last_time = time.time()
        print("Send:", cmd)
    except:
        pass

# ===== Đếm ngón tay =====
def count_fingers(hand_landmarks):
    tips = [4, 8, 12, 16, 20]
    fingers = []

    # Ngón cái (trục X)
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

# ===== LOOP =====
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

            # Lấy vị trí ngón trỏ
            x = int(lm[8].x * w)
            y = int(lm[8].y * h)

            cv2.circle(frame, (x, y), 10, (0,255,0), -1)

            # ===== LOGIC ĐIỀU KHIỂN =====
            if fingers >= 4:
                command = "STOP"
            else:
                # TRÁI / PHẢI
                if x < w * 0.3:
                    command = "LEFT"
                elif x > w * 0.7:
                    command = "RIGHT"
                else:
                    # TRÊN / DƯỚI
                    if y < h * 0.3:
                        command = "FORWARD"   # Đưa tay lên
                    elif y > h * 0.7:
                        command = "BACKWARD"  # Đưa tay xuống
                    else:
                        command = "STOP"

            send_command(command)

    # ===== VẼ VÙNG ĐIỀU KHIỂN =====
    cv2.rectangle(frame, (0,0), (int(w*0.3), h), (255,0,0), 2)           # LEFT
    cv2.rectangle(frame, (int(w*0.7),0), (w, h), (255,0,0), 2)           # RIGHT
    cv2.rectangle(frame, (0,0), (w, int(h*0.3)), (255,0,0), 2)           # FORWARD
    cv2.rectangle(frame, (0,int(h*0.7)), (w, h), (255,0,0), 2)           # BACKWARD

    # ===== HIỂN THỊ =====
    cv2.putText(frame, f"CMD: {command}", (10,50),
                cv2.FONT_HERSHEY_SIMPLEX, 1, (0,0,255), 2)

    cv2.imshow("Gesture Control", frame)

    if cv2.waitKey(1) & 0xFF == 27:
        break

# ===== CLEAN =====
cap.release()
cv2.destroyAllWindows()