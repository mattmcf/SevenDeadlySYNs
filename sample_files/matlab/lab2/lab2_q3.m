% Matt McFarland
% ENGS 91, Lab 2
% Question 3

function [  ] = lab2_q3(  )
    close all; clear all;
    
    maxlen = 50;
    threshold = 10e-10;
    
    angles = zeros(maxlen,2);
    dif = zeros(maxlen,2);
    delta = zeros(maxlen,2);
    convergence = zeros(maxlen,2);
    
    angles(1,:) = [30,.1];
    angles(1,:) = angles(1,:) .* pi / 180;   % turn into radians
    dif(1,:) = [1,1];
    i = 1;
    while (i<maxlen && (dif(i,1) > threshold && dif(i,2) > threshold))
        angles(i+1,:) = angles(i,:) - (J_inverse(angles(i,1),angles(i,2)) * F(angles(i,1),angles(i,2)))';
        i = i + 1;
        dif(i,:) = abs(angles(i,:) - angles(i-1,:));
    end

    for x = 2:i
        delta(x,1) = abs(angles(x,1) - angles(x-1,1));
        delta(x,2) = abs(angles(x,2) - angles(x-1,2));
    end

    theta2_root = angles(i,1);
    theta3_root = angles(i,2);
    for y = 1:i
        convergence(y,1) = abs(angles(y,1)-theta2_root);
        convergence(y,2) = abs(angles(y,2)-theta3_root);
    end
    
    figure(1)
    subplot(1,2,1)
    plot(1:i,angles(1:i,1)*180/(pi),'or', 1:i,angles(1:i,2)*180/(pi),'xb')
    legend('angle 2','angle 3')
    xlabel('n')
    ylabel('degrees')
    title('Root Approximation')
    
    subplot(1,2,2)
    semilogy(1:i,convergence(1:i,1),'--r', 1:i,convergence(1:i,2),'b')
    legend('angle 2','angle 3')
    ylabel('log(|P_n - P|')
    xlabel('n')
    title('Convergence Rate')
    
    str1 = ['Theta 2 is ', num2str(angles(i-1,1) * 180/pi)];
    disp(str1);
    
    str2 = ['Theta 3 is ', num2str(angles(i-1,2) * 180/pi)];
    disp(str2);
    
    F(angles(i-1,1),angles(i-1,2));
    
    iterations = ['Iterations: ',num2str(i)];
    disp(iterations);
end

% Takes radian arguments
function f1 = F(angle_2, angle_3) 
    a2_rad = angle_2;
    a3_rad = angle_3;
    
    r1 = 10;
    r2 = 6;
    r3 = 8;
    r4 = 4;

    angle_4 = 220;
    a4_rad = angle_4 * pi/180;  %radians

    f1(1,1) = r2*cos(a2_rad) + r3*cos(a3_rad) + r4*cos(a4_rad) - r1;
    f1(2,1) = r2*sin(a2_rad) + r3*sin(a2_rad) + r2*sin(a4_rad);
end

% Takes radian arguments
function out = J_inverse(angle_2, angle_3)
    out = zeros(2);

    r2 = 6;
    r3 = 8;

    a2 = angle_2; 
    a3 = angle_3;
    
    determinant = 1 / (r3*sin(a3)*r2*cos(a2) - r2*sin(a2)*r3*cos(a3));
    
    out(1,1) = r3*cos(a3) * determinant;
    out(1,2) = r3*sin(a3) * determinant;
    out(2,1) = -r2*cos(a2) * determinant;
    out(2,2) = -r2*sin(a2) * determinant;
end

    
