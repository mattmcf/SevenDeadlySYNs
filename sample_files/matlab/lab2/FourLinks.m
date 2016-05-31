% Takes radian arguments
function [a2, a3] = FourLinks(b1, b2, b3, b4, a1, a2_0, a3_0)
% Solves a 4 bar linkage problem for input a1 and bars b1...b4 and 
% initial guesses a2_0 and a3_0 for a2 and a3. Uses Newton's method of
% solving non-linear equations to find roots a2 and a3

    tolerance = 10e-12;  % iterates until solution is within 10e-6 of zero

    a4 = a1 + pi;
    max_iterations = 1000;

    angles = zeros(max_iterations,2);
    
    % initialize angles
    angles(1,1) = a2_0;
    angles(1,2) = a3_0;

    dif = zeros(max_iterations,2);
    dif(1,:) = abs(angles(1,:));
    
    i = 1;
    while ( (dif(i,1) > tolerance || dif(i,2) > tolerance) && ... 
            i < max_iterations)
        
        a2_cur = angles(i,1);
        a3_cur = angles(i,2);
  
        [f1, f2] = f(b3, a2_cur, b2, a3_cur, b1, a4, b4);
        
        % calculate next iteration
        sub = [angles(i,1),angles(i,2)] - ...
            (J_inverse(b2, a2_cur, b3, a3_cur) * [f1, f2]')';
        angles(i+1,1) = wrapTo2Pi(sub(1));
        angles(i+1,2) = wrapTo2Pi(sub(2));
        
        %[angles(i+1,1),angles(i+1,2)] = [angles(i,1),angles(i,2)] - ...
        %    (J_inverse(b2, a2_cur, b3, a3_cur) * [a2_calc, a3_calc]')';        
        
        i = i + 1;
        
        dif(i,:) = abs(angles(i,:) - angles(i-1,:));
    end
    
    a2 = angles(i,1);
    a3 = angles(i,2);
    
    % plot newton's method approximation to see convergence
    plot(1:i,angles(1:i,1),'x', 1:i,angles(1:i,2),'o');
end

% computes linkage equations
% both f1 and f2 should be equal to zero when a2 and a3 are correct
function [f1, f2] = f(bar3, a2, bar2, a3, bar1, a4, bar4)
    f1 = bar3*cos(a2) + bar2*cos(a3) + bar1*cos(a4) - bar4;
    f2 = bar3*sin(a2) + bar2*sin(a3) + bar1*sin(a4);
end

% Takes radian arguments
function out = J_inverse(bar2, a2, bar3, a3)
    out = zeros(2);
    
    %D = (-bar4*sin(a2)*bar3*cos(a3) + bar3*sin(a3)*bar4*cos(a2));
    D = bar2*bar3*sin(a3)*cos(a2) - bar2*bar3*sin(a2)*cos(a3);
    
    out(1,1) = bar2*cos(a3) / D;
    out(1,2) = bar2*sin(a3) / D;
    out(2,1) = -bar3*cos(a2) / D;
    out(2,2) = -bar3*sin(a2) / D;
end

function [ wrapped ] = wrapTo2Pi( angle )
% Take an angle (in radians) and wrap it to the output [0 2pi]
    n = floor(angle / (2*pi));
    wrapped = angle - n*2*pi;
end

