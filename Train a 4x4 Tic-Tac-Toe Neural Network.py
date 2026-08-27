import numpy as np
import pickle
import os
import random

def check_board(whose_turn, three, board):
    for row in range(three):
        won = True
        for col in range(three):
            if not board[row * three + col] == whose_turn:
                won = False
                break
        if won:
            return  True
    for col in range(three):
        won = True
        for row  in range(three):
            if not board[row * three + col] == whose_turn:
                won = False
                break
        if won:
            return  True
    col = 0
    row = 0
    won = True
    while row < three:
        if not board[row * three + col] == whose_turn:
            won = False
            break
        col += 1
        row += 1
    if won:
        return True
    col = three - 1
    row = 0
    won = True
    while row < three:
        if not board[row * three + col] == whose_turn:
            won = False
            break
        col -= 1
        row += 1
    if won:
        return True
    return False

board_side_length = 4
board_length = board_side_length**2

number_of_input_neurons = 2 * board_length
hidden_layers = [ 64, 32 ]
learning_rate = 0.1

biases = []
weights = []

biases_input_to_hidden = []
weights_input_to_hidden = []
for i in range(number_of_input_neurons):
    weights_input_to_hidden.append([])
    for j in range(hidden_layers[0]):
        weights_input_to_hidden[-1].append(random.random() - 0.5)

weights.append(np.array(weights_input_to_hidden))

biases_input_to_hidden.append([])
for i in range(hidden_layers[0]):
    biases_input_to_hidden[-1].append(random.random() - 0.5)

biases.append(np.array(biases_input_to_hidden))

weights_hidden_to_hidden = []
for i in range(hidden_layers[0]):
    weights_hidden_to_hidden.append([])
    for j in range(hidden_layers[1]):
        weights_hidden_to_hidden[-1].append(random.random() - 0.5)

weights.append(np.array(weights_hidden_to_hidden))

biases_hidden_to_hidden = []
biases_hidden_to_hidden.append([])

for i in range(hidden_layers[1]):
    biases_hidden_to_hidden[-1].append(random.random() - 0.5)

biases.append(np.array(biases_hidden_to_hidden))

weights_hidden_to_output = []
for i in range(hidden_layers[1]):
    weights_hidden_to_output.append([])
    for j in range(1):
        weights_hidden_to_output[-1].append(random.random() - 0.5)

weights.append(np.array(weights_hidden_to_output))

biases_hidden_to_output = []
biases_hidden_to_output.append([])
biases_hidden_to_output[-1].append(random.random() - 0.5)

biases.append(np.array(biases_hidden_to_output))

# Load saved model if it exists
if os.path.exists("model_data.pkl"):
    with open("model_data.pkl", "rb") as f:
        data = pickle.load(f)
        import numpy as np
        weights = data["weights"]
        biases = data["biases"]

naughts = 0
number_of_games_in_a_set = 0
number_of_games = 0

testing = 1
training = 2
evaluating = 3

mode = evaluating

inputs = []
outputs = []

test_inputs = []
test_outputs = []

smallest_testing_error = 1e9
initial_testing_error = -1

next_output = 1.0

while True:
    board = [0] * board_length
    input_crosses = [0] * number_of_input_neurons
    last_move_of_naughts = -1
    whose_turn = -1
    draw = False
    while True:
        empty_squares = []
        for i in range(len(board)):
            if board[i] == 0:
                empty_squares.append(i)
        if len(empty_squares) == 0:
            draw = True
            break
        whose_turn = -whose_turn
        best_move = empty_squares[0]
        if whose_turn == -1:
            best_move = random.choice(empty_squares)
        else:
            if not mode == evaluating:
                best_move = random.choice(empty_squares)
            else:
                best_score = -1
                for empty_square in empty_squares:
                    input_crosses[empty_square] = 1
                    z1 = np.dot(np.array([input_crosses]), weights[0]) + biases[0]
                    a1 = np.maximum(0, z1)
                    z2 = np.dot(a1, weights[1]) + biases[1]
                    a2 = np.maximum(0, z2)
                    z3 = np.dot(a2, weights[2]) + biases[2]
                    a3 = 1.0 / ( 1.0 + np.exp(-z3))
                    input_crosses[empty_square] = 0
                    if a3[0][0] > best_score:
                        best_score = a3[0][0]
                        best_move = empty_square
        board[best_move] = whose_turn
        if whose_turn == -1:
            input_crosses[best_move + board_length] = 1
            last_move_of_naughts = best_move + board_length
        else:
            input_crosses[best_move] = 1
        if check_board(whose_turn, board_side_length, board):
            break
    number_of_games += 1

    if mode == evaluating:
        number_of_games_in_a_set += 1
    output = 1.0
    if not draw and whose_turn == -1:
        output = 0.0
        if mode == evaluating:
            naughts += 1
    if number_of_games_in_a_set == 1000:
        print(f"number of games: {number_of_games} naughts: {naughts}")
        if naughts == 0:
            mode = evaluating
        elif len(test_outputs) == 0:
            mode = testing
        else:
            mode = training
        number_of_games_in_a_set = 0
        naughts = 0
    if not mode == evaluating:
        if next_output == 1.0:
            next_output = 0.0
        else:
            next_output = 1.0
        if whose_turn == -1:
            input_crosses[last_move_of_naughts] = 0
        inputs.append(input_crosses[:])
        outputs.append(output)
        if len(outputs) == 256:
            if mode == testing:
                test_outputs = outputs[:]
                for i in range(len(inputs)):
                    test_inputs.append(inputs[i][:])
                outputs = []
                inputs = []
                mode = training
            elif mode == training:
                inputs = np.array(inputs)
                z1 = np.dot(inputs, weights[0]) + biases[0]
                a1 = np.maximum(0, z1)
                z2 = np.dot(a1, weights[1]) + biases[1]
                a2 = np.maximum(0, z2)
                z3 = np.dot(a2, weights[2]) + biases[2]
                a3 = 1.0 / (1.0 + np.exp(-z3))
                dz3 = (a3 - np.array([outputs]).T)
                db3 = np.sum(dz3, axis=0, keepdims=True) / 256
                biases[2] -= learning_rate * db3
                dw3 = np.dot(a2.T, dz3) / 256
                dz2 = np.dot(dz3, weights[2].T) * (z2 > 0)
                weights[2] -= learning_rate * dw3
                db2 = np.sum(dz2, axis=0, keepdims=True) / 256
                biases[1] -= learning_rate * db2
                dw2 = np.dot(a1.T, dz2) / 256
                dz1 = np.dot(dz2, weights[1].T) * (z1 > 0)
                weights[1] -= learning_rate * dw2
                db1 = np.sum(dz1, axis=0, keepdims=True) / 256
                biases[0] -= learning_rate * db1
                dw1 = np.dot(inputs.T, dz1) / 256
                weights[0] -= learning_rate * dw1
                print(f"Training error: {np.sum(0.5 * (a3 - np.array([outputs]).T)**2)/256}")

                inputs = np.array(test_inputs)
                z1 = np.dot(inputs, weights[0]) + biases[0]
                a1 = np.maximum(0, z1)
                z2 = np.dot(a1, weights[1]) + biases[1]
                a2 = np.maximum(0, z2)
                z3 = np.dot(a2, weights[2]) + biases[2]
                a3 = 1.0 / (1.0 + np.exp(-z3))
                testing_error = np.sum(0.5 * (a3 - np.array([test_outputs]).T)**2)/256
                if initial_testing_error == -1:
                    initial_testing_error = testing_error
                if testing_error < smallest_testing_error:
                    smallest_testing_error = testing_error
                    with open("model_data.pkl", "wb") as f:
                        pickle.dump({"weights": weights, "biases": biases}, f)

                print(f"Testing error: {testing_error} smallest testing error: {smallest_testing_error} initial testing error: {initial_testing_error}")
                mode = evaluating

            inputs = []
            outputs = []
