# Hw3-5

## Implementation

#### store 10 captured values ( `temp[3]` )in 100ms and average it.

#### Using a new variable `during` which may take values for 8, 4, 2, 1.

#### if the total of the sum (`pitch`, `roll` and `yaw`) <= 0,

        the length of node would be 1/8, in other words, `freq` / 8 (`freq` / 8);

#### if the total of the sum (`pitch`, `roll` and `yaw`) >0, <= 2,

        the length of node would be 1/4, in other words, `freq` / 4

#### if the total of the sum (`pitch`, `roll` and `yaw`) >2, <=4,

        the length of node would be 1/2, in other words, `freq` / 2

#### if the total of the sum (`pitch`, `roll` and `yaw`) >4,

        the length of node would be 1, in other words, `freq` / 1

## Results

#### The length of the note differs with different value of `pitch`, `roll`, `yaw`
