% Matt McFarland
% ENGS 91, 15F, lab2

close all
clear all

%%% Question 2 - Part A %%%

maxlen = 1000;       % Iterations
first = -.79;         % start point
second = -.78;         % second start point (if needed)

% syms x;
% f = x + x*exp(-x) + cos(x)
% f1 = diff(f)
% f2 = diff(f1)
% 
% g = (x + x*exp(-x) + cos(x))^2
% g1 = diff(g)
% g2 = diff(g1)

f1 = @(x) (x + x.*exp(-x) + cos(x));
f1_prime = @(x) (1 + (exp(-x)).*(1-x) - sin(x));
f1_doubleprime = @(x) (-2*exp(-x) + x.*exp(-x) - cos(x));

next_newton = @(cur) (cur - (f1(cur) / f1_prime(cur)));
next_secant = @(cur,last) (cur - f1(cur)*(cur-last) / (f1(cur)-f1(last)) );

modded_u = @(x) (f1(x) / f1_prime(x));
modded_uprime = @(x) ((1 - (f1(x) .* f1_doubleprime(x)) / f1_prime(x)^2));
%next_modded_newton = @(cur) (cur - (modded_u(cur) / modded_uprime(cur)));
%next_modded_newton = @(cur) (cur - (f1(cur)*f1_prime(cur))/(f1_prime(cur)^2-(f1(cur)*f1_doubleprime(cur))/(f1_prime(cur)^2)));
next_modded_newton = @(cur) (cur - ( f1(cur)*f1_prime(cur) ) / ( (f1_prime(cur)^2) - f1_doubleprime(cur)*f1(cur)) );

Newton = zeros(1,maxlen);
Secant = zeros(1,maxlen);
Modded_Newton = zeros(1,maxlen);

newton_error = zeros(1,maxlen);
secant_error = zeros(1,maxlen);
modded_error = zeros(1,maxlen);

% { E_n+1, E_n, E_n-1, log(abs(E_n+1/E_n)), log(abs(E_n/E_n-1)) }
newton_alpha = zeros(5,maxlen);
secant_alpha = zeros(5,maxlen);
modded_alpha = zeros(5,maxlen);

Newton(1) = f1(first);
i = 1;
newton_error(1) = -1;
while (i < maxlen && newton_error(i) ~= 0)
    Newton(i+1) = next_newton(Newton(i));
    i = i + 1;
    newton_error(i) = abs(Newton(i)-Newton(i-1));
end
Newton_root = Newton(i);

% Calculate E_n for each iteration step -> alphas
for x = 3:(i-1)
    newton_alpha(1,x) = Newton(x+1) - Newton(x);    % E_n+1 / 
    newton_alpha(2,x) = Newton(x) - Newton(x-1);    % E_n  
    newton_alpha(3,x) = Newton(x-1) - Newton(x-2);  % E_n-1
    newton_alpha(4,x) = log(abs(newton_alpha(1,x)./newton_alpha(2,x)));
    newton_alpha(5,x) = log(abs(newton_alpha(2,x)./newton_alpha(3,x)));
end

Secant(1) = f1(first);
Secant(2) = f1(second);
secant_error(2) = abs(Secant(2) - Secant(1));
j = 2;
while (j < maxlen && secant_error(j) ~= 0)
    Secant(j+1) = next_secant(Secant(j),Secant(j-1));
    j = j + 1;
    secant_error(j) = abs(Secant(j) - Secant(j-1));
end
Secant_root = Secant(j);

for x = 3:(j-1)
    secant_alpha(1,x) = Secant(x+1) - Secant(x);    % E_n+1 
    secant_alpha(2,x) = Secant(x) - Secant(x-1);    % E_n  
    secant_alpha(3,x) = Secant(x-1) - Secant(x-2);  % E_n-1
    secant_alpha(4,x) = log(abs(secant_alpha(1,x)./secant_alpha(2,x)));
    secant_alpha(5,x) = log(abs(secant_alpha(2,x)./secant_alpha(3,x)));
end

Modded_Newton(1) = f1(first);
k = 1;
modded_error(1) = -1;
while (k < maxlen && modded_error(k) ~= 0)
    Modded_Newton(k+1) = next_modded_newton(Modded_Newton(k));
    k = k + 1;
    modded_error(k) = abs(Modded_Newton(k) - Modded_Newton(k-1));
end
Modded_root = Modded_Newton(k);

for x = 3:(k-1)
    modded_alpha(1,x) = Modded_Newton(x+1) - Modded_Newton(x);    % E_n+1 
    modded_alpha(2,x) = Modded_Newton(x) - Modded_Newton(x-1);    % E_n  
    modded_alpha(3,x) = Modded_Newton(x-1) - Modded_Newton(x-2);  % E_n-1
    modded_alpha(4,x) = log(abs(modded_alpha(1,x)./modded_alpha(2,x)));
    modded_alpha(5,x) = log(abs(modded_alpha(2,x)./modded_alpha(3,x)));
end

newton_summary = ['Newton Root: ',num2str(Newton_root),...
    ' -- iterations: ',num2str(i)];

secant_summary = ['Secant Root: ',num2str(Secant_root),...
    ' -- iterations: ',num2str(j)];

modded_summary = ['Modified Newtons Method Root: ',num2str(Modded_root),...
    ' -- iterations: ',num2str(k)];

figure('name','Question 2, part 1')
figure(1)
plot(newton_alpha(5,:),newton_alpha(4,:),'or',...
        secant_alpha(5,:),secant_alpha(4,:),'xb',...
        modded_alpha(5,:),modded_alpha(4,:),'.k');
grid on
xlabel('log(|E n / E (n-1)|)')
ylabel('log(|E (n+1) / E n|)')
legend('Newton','Secant','Modified Newton','Location','southwest')
title('Slope Showing The Rate of Convergence')    

figure('name','Question 2, part 1')
subplot(1,2,1)
plot(1:i,Newton(1:i),'ob', 1:j,Secant(1:j),'rx', 1:k,Modded_Newton(1:k), '.k')
title('Root Approximation')
xlabel('n')
ylabel('Root Value')
legend('Newton','Secant','Modfied Newton','Location','northeast')

subplot(1,2,2)
semilogy(2:i,abs(Newton(2:i)-Newton_root),'-b', ...
    2:j,abs(Secant(2:j)-Secant_root),'--r', ...
    2:k,abs(Modded_Newton(2:k)-Modded_root),'-.k');
title('Approximation Convergence')
xlabel('n')
ylabel('log(|P_n-P|)')
legend('Newton','Secant','Modfied Newton','Location','southwest')


disp(newton_summary)
disp(secant_summary)
disp(modded_summary)