my = open('result.csv')
real = open('hw4_data/y_test', 'rb')

score = 0
for line in my:
    if line.startswith('id,label'):
        continue

    my_fig = int(line.split(',')[1])
    real_fig = real.read(1)[0]

    if my_fig == real_fig:
        score += 1

print(score / 10000)

# learning_rate | num_iteration | accuracy
# 100 2 0.6804
# 100 3 0.6804
# 100 4 0.5216
# 100 5 0.3204
# 100 6 0.3746
# 100 7 0.3563
# 0.0004 9 0.4042
